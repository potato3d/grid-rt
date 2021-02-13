#include <rtp/KdTreeAccStruct.h>
#include <rt/Context.h>
#include <rt/AabbIntersection.h>
#include <rt/RayTriIntersection.h>
#include <rt/IEnvironment.h>

using namespace rtp;

void KdNode::setInternalNode( const rt::SplitPlane& plane, const KdNode* leftChild )
{
	// Store plane information
	_data = plane.axis;
	_split = plane.position;

	// Store pointer to left child
#pragma warning( disable : 4311 )
	_data += reinterpret_cast<uint32>( leftChild );
#pragma warning( default : 4311 )

	// Reset leaf flag
	_data &= 0x7FFFFFFF;
}

void KdNode::setLeafNode( uint32 elementStart, uint32 elementCount )
{
	// Store start of elements
	_elements = elementStart;

	// Store number of elements
	_data = elementCount;

	// Set leaf flag
	_data |= 0x80000000;
}

// Static and thread-safe traversal stacks
__declspec(thread) static KdTreeAccStruct::TraversalStack s_instanceStack;
__declspec(thread) static KdTreeAccStruct::TraversalStack s_geometryStack;

KdTreeAccStruct::KdTreeAccStruct()
: root( NULL ), elements( NULL )
{
}

void KdTreeAccStruct::clear()
{
	if( root != NULL )
		delete [] root;
	if( elements != NULL )
		delete [] elements;
}

void KdTreeAccStruct::traceNearestInstance( const std::vector<rt::Instance>& instances, rt::Sample& sample )
{
	rt::Ray& ray = sample.ray;
	rt::Hit& hit = sample.hit;

	rt::Context* ctx = rt::Context::current();

	// Init ray
	ray.tnear = ctx->getRayEpsilon();
	ray.tfar = vr::Mathf::MAX_VALUE;
	ray.update();

	// Init hit
	hit.instance = NULL;
	hit.distance = vr::Mathf::MAX_VALUE;

	// If not hit bbox of entire scene, no need to trace any further
	if( !rt::AabbIntersection::clipRay( _bbox, ray ) )
	{
		ctx->getEnvironment()->shade( sample );
		return;
	}

	const rt::Ray originalRay( ray );
	const KdNode* node = root;
	s_instanceStack.clear();

	while( true )
	{
		findLeaf( node, ray, s_instanceStack );

		for( uint32 i = node->elemStart(), limit = i + node->elemCount(); i < limit; ++i )
		{
			const rt::Instance& instance = instances[elements[i]];

			// Transform ray to geometry's local space
			instance.transform.inverseTransform( ray );
			ray.update();

			// Ask geometry's acceleration structure to trace the transformed ray
			instance.geometry->accStruct->traceNearestGeometry( instance, ray, hit );

			// Transform ray back to global space
			ray = originalRay;
		}

		// If found hit, return
		if( hit.instance )
		{
			hit.instance->geometry->triDesc[hit.triangleId].material->shade( sample );
			return;
		}

		// If no more nodes to traverse, return
		if( s_instanceStack.empty() )
		{
			ctx->getEnvironment()->shade( sample );
			return;
		}

		// Continue traversal
		const TraversalData& data = s_instanceStack.top();
		s_instanceStack.pop();
		node = data.node;
		ray.tnear = data.tnear;
		ray.tfar = data.tfar;
	}
}

void KdTreeAccStruct::traceNearestGeometry( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit )
{
	// If don't hit bbox in local space, no need to trace underlying triangles
	if( !rt::AabbIntersection::clipRay( _bbox, ray ) )
		return;

	const rt::Geometry& geometry = *instance.geometry;
	const KdNode* node = root;
	float bestDistance = hit.distance;
	s_geometryStack.clear();

	while( true )
	{
		findLeaf( node, ray, s_geometryStack );

		for( uint32 i = node->elemStart(), limit = i + node->elemCount(); i < limit; ++i )
		{
			rt::RayTriIntersection::hitWald( geometry.triAccel[elements[i]], ray, hit, bestDistance );
		}

		// If found hit, return
		if( bestDistance < hit.distance )
		{
			hit.distance = bestDistance;
			hit.instance = &instance;
			return;
		}

		// If no more nodes to traverse, return
		if( s_geometryStack.empty() )
			return;

		// Continue traversal
		const TraversalData& data = s_geometryStack.top();
		s_geometryStack.pop();
		node = data.node;
		ray.tnear = data.tnear;
		ray.tfar = data.tfar;
	}
}

// Private
void KdTreeAccStruct::findLeaf( const KdNode*& node, rt::Ray& ray, TraversalStack& stack )
{
	while( !node->isLeaf() )
	{
		// Need to avoid NaN's when split - ray.origin[axis] == 0 and ray.invDir[axis] == +/- INF
		const RTenum axis = node->axis();
		const float d = ( node->splitPos() - ray.orig[axis] ) * ray.invDir[axis];

		const uint32 bit = ray.dirSignBits[axis];

		const KdNode* const front = node->leftChild() + bit;
		const KdNode* const back = node->leftChild() + !bit;

		// Using < and > instead of <= and >= because of flat cells and triangles in the split plane.
		// In this case, we must traverse both children to guarantee that we hit the triangle we want.
		if( d < ray.tnear )
		{
			node = back;
		}
		else if( d > ray.tfar )
		{
			node = front;
		}
		else
		{
			stack.push();
			TraversalData& data = stack.top();

			// Store far child for later traversal
			data.node  = back;
			//data.tnear = d;
			data.tnear = ( d > ray.tnear ) ? d : ray.tnear; // avoid NaNs
			data.tfar  = ray.tfar;

			// Continue with front child
			node = front;
			//ray.tfar = d;
			ray.tfar = ( d < ray.tfar ) ? d : ray.tfar; // avoid NaNs
		}
	}
	
	// alternative version by tbp & phantom (2005)
	// approx. 26 cycles in K8
	/*
    while (!node->is_leaf()) 
    {
    	const uint_t	axis = node->get_axis();
    	const float
    		split	= node->inner.split_coord,
    		d	= (split - ray.pos[axis]) * ray.inv_dir[axis];
    
    	const bool_t negative = direction_signs[axis];
    
    	const kdtree::node_t
    		* const left  = node->get_back(),
    		* const right = left + 1;
    
    	const bool_t
    		backside	= d <= rs.t_near,	// case one, d <= t_near <= t_far -> cull front side
    		frontside	= d >= rs.t_far,	// case two, t_near <= t_far <= d -> cull back side
    		skip		= backside | frontside,
    		other		= negative^frontside;
    
    	
    	// here's a denser version of what happens to the node.
    	// node = skip ? (other ? left : right) : (negative ? right : left);
    	if (!skip) 
    	{
    		stack.push(negative ? left : right, d, rs.t_far);
    		rs.t_far = d;
    		node = negative ? right : left;
    	}
    	else 
    		node = other ? left : right;
    }
    */
}
