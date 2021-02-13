#include <rt/AabbIntersection.h>

using namespace rt;

void AabbIntersection::clipTriangle( const vr::vec3f& v0, const vr::vec3f& v1, const vr::vec3f& v2, 
							         const Aabb& box, Aabb& result )
{
	// Sutherland-Hodgman Clipping
	vr::vec3f vertexBuffer[8];
	vr::vec3f tempBuffer[8];

	vertexBuffer[0] = v0;
	vertexBuffer[1] = v1;
	vertexBuffer[2] = v2;

	uint32 vertexCount = 3;

	clip( vertexBuffer, tempBuffer, vertexCount, box.minv.x, 1.0f, RT_AXIS_X );
	clip( vertexBuffer, tempBuffer, vertexCount, box.minv.y, 1.0f, RT_AXIS_Y );
	clip( vertexBuffer, tempBuffer, vertexCount, box.minv.z, 1.0f, RT_AXIS_Z );
	clip( vertexBuffer, tempBuffer, vertexCount, box.maxv.x, -1.0f, RT_AXIS_X );
	clip( vertexBuffer, tempBuffer, vertexCount, box.maxv.y, -1.0f, RT_AXIS_Y );
	clip( vertexBuffer, tempBuffer, vertexCount, box.maxv.z, -1.0f, RT_AXIS_Z );

	result.buildFrom( vertexBuffer, vertexCount );
}

void AabbIntersection::splitAabb( const Aabb& box, const SplitPlane& plane, Aabb& left, Aabb& right )
{
	// If plane does not split box in two, one of the resulting Aabbs will be degenerated
	left = box;
	right = box;
	left.maxv[plane.axis] = vr::min( plane.position, box.maxv[plane.axis] );
	right.minv[plane.axis] = vr::max( plane.position, box.minv[plane.axis] );
}

bool AabbIntersection::clipRay( const Aabb& box, Ray& ray )
{
	float tNear;
	float tFar;
	// Only update ray if found valid clipping
	float rayNear = ray.tnear;
	float rayFar = ray.tfar;

	for( uint32 i = 0; i < 3; ++i )
	{
		// Update interval for ith bounding box slab
		tNear = ( box.minv[i] - ray.orig[i] ) * ray.invDir[i];
		tFar  = ( box.maxv[i] - ray.orig[i] ) * ray.invDir[i];

		// Update parametric interval from slab intersection tnear/tfar
		rayNear = vr::max( rayNear, vr::min( tNear, tFar ) );
		rayFar = vr::min( rayFar, vr::max( tNear, tFar ) );
	}

	if( rayNear > rayFar )
		return false;

	// Update ray interval
	ray.tnear = rayNear;
	ray.tfar = rayFar;
	return true;
}

bool AabbIntersection::triangleOverlaps( const Aabb& box, const vr::vec3f& v0, 
									   const vr::vec3f& v1, const vr::vec3f& v2 )
{
	// Tomas Akenine-Möller Aabb-triangle overlap test (author's optimized and fixed version 18-06-2001)
	// http://www.cs.lth.se/home/Tomas_Akenine_Moller/code/tribox3.txt

	// Use separating axis theorem to test overlap between triangle and box
	// Need to test for overlap in these cases (directions):
	// 1) the {x,y,z}-directions (actually, since we use the Aabb of the triangle we do not even need to test these)
	// 2) normal of the triangle
	// 3) crossproduct(edge from tri, {x,y,z}-direction), this gives 3x3=9 more tests

	// Compute box center and halfsizes
	const vr::vec3f boxCenter = ( box.maxv + box.minv ) * 0.5f;
	const vr::vec3f boxHalfSizes = ( box.maxv - box.minv ) * 0.5f;

	// This is the fastest branch on Sun
	// Move everything so that the boxcenter is in (0,0,0)
	const vr::vec3f vv0 = v0 - boxCenter;
	const vr::vec3f vv1 = v1 - boxCenter;
	const vr::vec3f vv2 = v2 - boxCenter;

	// Compute triangle edges
	const vr::vec3f e0 = vv1 - vv0;
	const vr::vec3f e1 = vv2 - vv1;
	const vr::vec3f e2 = vv0 - vv2;

	// Case 3: test the 9 tests first (this was faster)
	float minp;
	float maxp;
	float rad;

	// Edge 0
	float fex = vr::abs( e0.x );
	float fey = vr::abs( e0.y );
	float fez = vr::abs( e0.z );

	axisTestX( boxHalfSizes, vv0, vv2, e0.z, e0.y, fez, fey, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestY( boxHalfSizes, vv0, vv2, e0.z, e0.x, fez, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestZ( boxHalfSizes, vv1, vv2, e0.y, e0.x, fey, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	// Edge 1
	fex = vr::abs( e1.x );
	fey = vr::abs( e1.y );
	fez = vr::abs( e1.z );

	axisTestX( boxHalfSizes, vv0, vv2, e1.z, e1.y, fez, fey, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestY( boxHalfSizes, vv0, vv2, e1.z, e1.x, fez, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestZ( boxHalfSizes, vv0, vv1, e1.y, e1.x, fey, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	// Edge 2
	fex = vr::abs( e2.x );
	fey = vr::abs( e2.y );
	fez = vr::abs( e2.z );

	axisTestX( boxHalfSizes, vv0, vv1, e2.z, e2.y, fez, fey, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestY( boxHalfSizes, vv0, vv1, e2.z, e2.x, fez, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestZ( boxHalfSizes, vv1, vv2, e2.y, e2.x, fey, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	// Case 1: first test overlap in the {x,y,z}-directions
	// Find min, max of the triangle each direction, and test for overlap in that direction 
	// this is equivalent to testing a minimal Aabb around the triangle against the Aabb

	// Test in X-direction
	findMinMax( vv0.x, vv1.x, vv2.x, minp, maxp );
	if( minp > boxHalfSizes.x || maxp < -boxHalfSizes.x )
		return false;

	// Test in Y-direction
	findMinMax( vv0.y, vv1.y, vv2.y, minp, maxp );
	if( minp > boxHalfSizes.y || maxp < -boxHalfSizes.y )
		return false;

	// Test in Z-direction
	findMinMax( vv0.z, vv1.z, vv2.z, minp, maxp );
	if( minp > boxHalfSizes.z || maxp < -boxHalfSizes.z )
		return false;

	// Case 2: test if the box intersects the plane of the triangle
	// compute plane equation of triangle: normal*x+d=0
	const vr::vec3f normal = e0.cross( e1 );

	// -NJMP- (line removed here)

	if( !planeBoxOverlap( boxHalfSizes, normal, vv0 ) ) // -NJMP-
		return false;

	return true;
}

bool AabbIntersection::isPointInside( const Aabb& box, const vr::vec3f& point )
{
	return ( point.x >= box.minv.x ) && ( point.y >= box.minv.y ) && ( point.z >= box.minv.z ) &&
		   ( point.x <= box.maxv.x ) && ( point.y <= box.maxv.y ) && ( point.z <= box.maxv.z );
}

/************************************************************************/
/* Private                                                              */
/************************************************************************/
void AabbIntersection::clip( vr::vec3f vertexBuffer[8], vr::vec3f tempBuffer[8], 
					 		 uint32& vertexCount, float pos, float dir, RTenum dim )
{
	bool allin = true;
	bool allout = true;

	// Try to accept or reject all vertices
	for( uint32 i = 0; i < vertexCount; ++i )
	{
		float dist = dir * ( vertexBuffer[i][dim] - pos );
		if( dist < 0 )
			allin = false;
		else
			allout = false;
	}

	// Check if all vertices were accepted or rejected
	if( allin )
		return;

	if( allout )
	{
		vertexCount = 0;
		return;
	}

	// Need to add each vertex and potential intersection points
	vr::vec3f v1 = vertexBuffer[0];
	float d1 = dir * ( v1[dim] - pos );
	bool inside = ( d1 >= 0 );
	uint32 count = 0;

	for( uint32 i = 0; i < vertexCount; ++i )
	{
		const vr::vec3f v2 = vertexBuffer[(i + 1) % vertexCount];
		float d2 = dir * ( v2[dim] - pos );

		if( inside && ( d2 >= 0 ) ) 
		{
			// Previous and current are inside, add current (assume first has been added)
			tempBuffer[count++] = v2;
		}
		else if( !inside && ( d2 >= 0 ) )
		{
			// Previous outside and current inside, add intersection point and then current
			float d = d1 / (d1 - d2);
			vr::vec3f vc = v1 + ( (v2 - v1) * d );
			vc[dim] = pos;
			tempBuffer[count++] = vc;
			tempBuffer[count++] = v2;
			inside = true;
		}
		else if( inside && ( d2 < 0 ) )
		{
			// Previous inside and current outside, add intersection point
			float d = d2 / (d2 - d1);
			vr::vec3f vc = v2 + ( (v1 - v2) * d );
			vc[dim] = pos;
			tempBuffer[count++] = vc;
			inside = false;
		}
		// Update previous vertex info
		v1 = v2;
		d1 = d2;
	}

	// New vertex count
	vertexCount = 0;

	for( uint32 i = 0; i < count; i++ )
	{
		const vr::vec3f dist = tempBuffer[i] - tempBuffer[(i + count - 1) % count];
		if( dist.length() > vr::Mathf::ZERO_TOLERANCE )
			vertexBuffer[vertexCount++] = tempBuffer[i];
	}
}

void AabbIntersection::axisTestX( const vr::vec3f& boxHalfSizes, const vr::vec3f& vA, const vr::vec3f& vB, 
								  float a, float b, float fa, float fb, float& minp, float& maxp, float& rad )
{
	const float p0 = a*vA.y - b*vA.z;
	const float p2 = a*vB.y - b*vB.z;

	if( p0 < p2 )
	{
		minp=p0;
		maxp=p2;
	}
	else
	{
		minp=p2;
		maxp=p0;
	}

	rad = fa * boxHalfSizes.y + fb * boxHalfSizes.z;
}

void AabbIntersection::axisTestY( const vr::vec3f& boxHalfSizes, const vr::vec3f& vA, const vr::vec3f& vB, 
								  float a, float b, float fa, float fb, float& minp, float& maxp, float& rad )
{
	const float p0 = -a*vA.x + b*vA.z;
	const float p2 = -a*vB.x + b*vB.z;

	if( p0 < p2 )
	{
		minp = p0;
		maxp = p2;
	}
	else
	{
		minp = p2;
		maxp = p0;
	}

	rad = fa * boxHalfSizes.x + fb * boxHalfSizes.z;
}

void AabbIntersection::axisTestZ( const vr::vec3f& boxHalfSizes, const vr::vec3f& vA, const vr::vec3f& vB, 
								  float a, float b, float fa, float fb, float& minp, float& maxp, float& rad )
{
	const float p1 = a*vA.x - b*vA.y;
	const float p2 = a*vB.x - b*vB.y;

	if( p2 < p1 )
	{
		minp = p2;
		maxp = p1;
	}
	else
	{
		minp = p1;
		maxp = p2;
	}

	rad = fa * boxHalfSizes.x + fb * boxHalfSizes.y;
}

void AabbIntersection::findMinMax( float a, float b, float c, float& minp, float& maxp )
{
	minp = a;
	maxp = a;

	if( b < minp )
		minp = b;
	if( b > maxp )
		maxp = b;
	if( c < minp )
		minp = c;
	if( c > maxp )
		maxp = c;
}

bool AabbIntersection::planeBoxOverlap( const vr::vec3f& boxHalfSizes, const vr::vec3f& normal, 
									    const vr::vec3f& vertex )
{
	vr::vec3f vmin;
	vr::vec3f vmax;

	for( RTenum d = RT_AXIS_X; d <= RT_AXIS_Z; ++d )
	{
		float v = vertex[d];					// -NJMP-

		if( normal[d] > 0.0f )
		{
			vmin[d] = -boxHalfSizes[d] - v;	// -NJMP-
			vmax[d] =  boxHalfSizes[d] - v;	// -NJMP-
		}
		else
		{
			vmin[d] =  boxHalfSizes[d] - v;	// -NJMP-
			vmax[d] = -boxHalfSizes[d] - v;	// -NJMP-
		}
	}
	if( normal.dot( vmin ) > 0.0f ) // -NJMP-
		return false;

	if( normal.dot( vmax ) >= 0.0f ) // -NJMP-
		return true;

	return false;
}
