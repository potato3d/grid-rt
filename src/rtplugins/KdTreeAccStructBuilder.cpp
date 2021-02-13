#include <rtp/KdTreeAccStructBuilder.h>
#include <rtp/KdTreeAccStruct.h>
#include <TriangleTreeBuilder.h>
#include <InstanceTreeBuilder.h>
#include <queue>

using namespace rtp;

KdTreeAccStructBuilder::KdTreeAccStructBuilder()
: _triangleTreeBuilder( new TriangleTreeBuilder ), _instanceTreeBuilder( new InstanceTreeBuilder )
{
	// empty
}

KdTreeAccStructBuilder::~KdTreeAccStructBuilder()
{
	delete _triangleTreeBuilder;
	delete _instanceTreeBuilder;
}

void KdTreeAccStructBuilder::buildGeometry( rt::Geometry* geometry )
{
	// Create kd-Tree using current geometry data. Ref_ptr will delete object in the end of this method.
	vr::ref_ptr<RawKdTree> tree = _triangleTreeBuilder->buildTree( geometry );

	// Create accelerated kd tree for ray tracing
	geometry->accStruct = convertRawTree( tree.get() );
}

rt::IAccStruct* KdTreeAccStructBuilder::buildInstance( const std::vector<rt::Instance> instances )
{
	// Rebuild instance kd tree
	vr::ref_ptr<RawKdTree> tree = _instanceTreeBuilder->buildTree( instances );

	// Create accelerated kd tree for ray tracing
	return convertRawTree( tree.get() );
}

KdTreeAccStruct* KdTreeAccStructBuilder::convertRawTree( RawKdTree* tree )
{
	// Target tree
	KdTreeAccStruct* result = new KdTreeAccStruct();

	// Store bounding box
	result->setBoundingBox( tree->bbox );

	// Create optimized nodes
	result->root = new KdNode[tree->stats.nodeCount];

	// Create triangle ids
	result->elements = new uint32[tree->stats.elemIdCount];

	// Store triangle ids and setup optimized nodes
	uint32 dstNode = 0;
	uint32 dstElemId = 0;
	uint32 elemIdCount = 0;
    uint32 childId = 1;

	RawKdNode* current;
	std::queue<RawKdNode*> next;
	next.push( tree->root.get() );

	while( !next.empty() )
	{
		current = next.front();
		next.pop();

		if( !current->isLeaf() )
		{
			result->root[dstNode].setInternalNode( current->split, &result->root[childId] );
			next.push( current->left.get() );
			next.push( current->right.get() );

			// Update variables for next iteration
            childId += 2;
            ++dstNode;
		}
		else
		{
			// Store element ids
			elemIdCount = current->elements.size();
			for( uint32 t = 0; t < elemIdCount; ++t )
			{
				result->elements[dstElemId+t] = current->elements[t];
			}

			// Update optimized node
			result->root[dstNode].setLeafNode( dstElemId, elemIdCount );

			// Update variables for next iteration
			++dstNode;
			dstElemId += elemIdCount;
		}
	}

	return result;
}
