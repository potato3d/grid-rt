#include "RawKdTree.h"

using namespace rtp;

RawKdNode::RawKdNode()
: left( NULL ), right( NULL )
{
	// empty
}

RawKdNode::RawKdNode( const RawKdNode::Elements& ids )
: left( NULL ), right( NULL ), elements( ids )
{
	// empty
}

RawKdNode::RawKdNode( const rt::SplitPlane& plane, RawKdNode* leftChild, RawKdNode* rightChild )
: split( plane ), left( leftChild ), right( rightChild )
{
	// empty
}

//////////////////////////////////////////////////////////////////////////
RawKdTree::Statistics::Statistics()
{
	reset();
}

void RawKdTree::Statistics::reset()
{
	nodeCount = 0;
	leafCount = 0;
	treeDepth = 0;
	elemIdCount = 0;
}
