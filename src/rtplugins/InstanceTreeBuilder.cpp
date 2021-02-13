#include "InstanceTreeBuilder.h"
#include <rt/AabbIntersection.h>

using namespace rtp;

RawKdTree* InstanceTreeBuilder::buildTree( const std::vector<rt::Instance>& instances )
{
	// Create instance vector for entire scene
	uint32 instanceCount = instances.size();

	if( instanceCount == 0 )
		return new RawKdTree();

	RawKdNode::Elements initialInstances( instanceCount );
	_instances = &instances;

	RawKdTree* tree = new RawKdTree();
	_stats = &tree->stats;
	_stats->reset();

	rt::Aabb& sceneBox = tree->bbox;
	sceneBox = instances[0].bbox;
	initialInstances[0] = 0;

	// Store instance ids and compute scene bounding box
	for( uint32 i = 1; i < instanceCount; ++i )
	{
		initialInstances[i] = i;
		sceneBox.expandBy( instances[i].bbox );
	}

	// Recursive tree build
	tree->root = recursiveBuild( initialInstances, tree->bbox, 0 );
	return tree;
}

// Private methods

RawKdNode* InstanceTreeBuilder::recursiveBuild( const RawKdNode::Elements& instances, const rt::Aabb& bbox, uint32 treeDepth )
{
	++_stats->nodeCount;

	// Check trivial case
	if( instances.size() == 1 )
		return leafNode( instances, treeDepth );
	
	// Find axis in descending order of maximum extent
	uint32 orderedAxis[3];
	orderAxis( orderedAxis, bbox );

	uint32 axis;
	rt::SplitPlane plane;
	float distance;
	uint32 currentInstanceCount = instances.size();
	const std::vector<rt::Instance>& originalInstances = *_instances;

	// Best split data
	float minDistance = vr::Mathf::MAX_VALUE;
	uint32 bestInstance;

	float leftMinDistance = vr::Mathf::MAX_VALUE;
	uint32 leftBestInstance;
	uint32 leftCount = 0;
	float leftSplitPosition;

	float rightMinDistance = vr::Mathf::MAX_VALUE;
	uint32 rightBestInstance;
	uint32 rightCount = 0;
	float rightSplitPosition;

	// Compute the center of current bbox
	vr::vec3f center = bbox.minv + bbox.maxv;
	center *= 0.5f;

	// Child elements, if any
	RawKdNode::Elements leftInstances;
	RawKdNode::Elements rightInstances;

	// For each axis
	for( uint32 k = 0; k < 3; ++k )
	{
		axis = orderedAxis[k];
		leftInstances.clear();
		rightInstances.clear();

		// Get "best" split position: closest object border to center of bbox
		for( uint32 i = 0; i < currentInstanceCount; ++i )
		{
			const uint32 instanceId = instances[i];
			const rt::Aabb& currentBox = originalInstances[instanceId].bbox;

			distance = currentBox.minv[axis] - center[axis];
			if( distance <= 0.0f )
			{
				++leftCount;
				distance = vr::abs( distance );
				if( distance < leftMinDistance )
				{
					leftMinDistance = distance;
					leftBestInstance = instanceId;
					leftSplitPosition = currentBox.minv[axis];
				}
			}
			else
			{
				++rightCount;
				distance = vr::abs( distance );
				if( distance < rightMinDistance )
				{
					rightMinDistance = distance;
					rightBestInstance = instanceId;
					rightSplitPosition = currentBox.minv[axis];
				}
			}

			distance = currentBox.maxv[axis] - center[axis];
			if( distance <= 0.0f )
			{
				++leftCount;
				distance = vr::abs( distance );
				if( distance < leftMinDistance )
				{
					leftMinDistance = distance;
					leftBestInstance = instanceId;
					leftSplitPosition = currentBox.maxv[axis];
				}
			}
			else
			{
				++rightCount;
				distance = vr::abs( distance );
				if( distance < rightMinDistance )
				{
					rightMinDistance = distance;
					rightBestInstance = instanceId;
					rightSplitPosition = currentBox.maxv[axis];
				}
			}
		}

		// Get the split position that lies between cell center and object median
		if( leftCount > rightCount )
		{
			minDistance = leftMinDistance;
			bestInstance = leftBestInstance;
			plane.position = leftSplitPosition;
		}
		else
		{
			minDistance = rightMinDistance;
			bestInstance = rightBestInstance;
			plane.position = rightSplitPosition;
		}

		// Partition instances according to best split position
		for( uint32 i = 0; i < currentInstanceCount; ++i )
		{
			const uint32 instanceId = instances[i];
			const rt::Aabb& currentBox = originalInstances[instanceId].bbox;

			if( ( currentBox.minv[axis] <= plane.position ) && ( currentBox.maxv[axis] <= plane.position ) )
			{
				// Completely on left side
				leftInstances.push_back( instanceId );
			}
			else if( ( currentBox.minv[axis] >= plane.position ) && ( currentBox.maxv[axis] >= plane.position ) )
			{
				// Completely on right side
				rightInstances.push_back( instanceId );
			}
			else
			{
				// On both sides
				leftInstances.push_back( instanceId );
				rightInstances.push_back( instanceId );
			}
		}

		// Check if best split was able to partition instances
		if( ( leftInstances.size() >= currentInstanceCount ) || ( rightInstances.size() >= currentInstanceCount ) )
			continue;

		// Split current bounding box according to chosen split plane
		plane.axis = axis;
		rt::Aabb leftBox;
		rt::Aabb rightBox;
		rt::AabbIntersection::splitAabb( bbox, plane, leftBox, rightBox );

		// Recursive tree build for both children
		RawKdNode* left  = recursiveBuild( leftInstances, leftBox, treeDepth + 1 );
		vr::vectorFreeMemory( leftInstances );
		RawKdNode* right = recursiveBuild( rightInstances, rightBox, treeDepth + 1 );
		vr::vectorFreeMemory( rightInstances );

		return new RawKdNode( plane, left, right );
	}

	// No valid split plane could be found
	return leafNode( instances, treeDepth );
}

void InstanceTreeBuilder::orderAxis( uint32* axis, const rt::Aabb& bbox )
{
	// Find axis in descending order of maximum extent
	vr::vec3f diagonal = bbox.maxv - bbox.minv;

	if( diagonal.x > diagonal.y )
	{
		if( diagonal.x > diagonal.z )
		{
			axis[0] = RT_AXIS_X;
			if( diagonal.y > diagonal.z )
			{
				axis[1] = RT_AXIS_Y;
				axis[2] = RT_AXIS_Z;
			}
			else\
			{
				axis[1] = RT_AXIS_Z;
				axis[2] = RT_AXIS_Y;
			}
		}
		else
		{
			axis[0] = RT_AXIS_Z;
			axis[1] = RT_AXIS_X;
			axis[2] = RT_AXIS_Y;
		}
	}
	else
	{
		if( diagonal.y > diagonal.z )
		{
			axis[0] = RT_AXIS_Y;
			if( diagonal.x > diagonal.z )
			{
				axis[1] = RT_AXIS_X;
				axis[2] = RT_AXIS_Z;
			}
			else
			{
				axis[1] = RT_AXIS_Z;
				axis[2] = RT_AXIS_X;
			}
		}
		else
		{
			axis[0] = RT_AXIS_Z;
			axis[1] = RT_AXIS_Y;
			axis[2] = RT_AXIS_X;
		}
	}
}

RawKdNode* InstanceTreeBuilder::leafNode( const RawKdNode::Elements& instances, uint32 treeDepth )
{
	if( treeDepth > _stats->treeDepth )
		_stats->treeDepth = treeDepth;

	_stats->elemIdCount += instances.size();
	++_stats->leafCount;

	return new RawKdNode( instances );
}
