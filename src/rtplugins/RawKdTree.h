#ifndef _RTP_RAWKDTREE_H_
#define _RTP_RAWKDTREE_H_

#include <rt/Aabb.h>
#include <rt/SplitPlane.h>
#include <vr/ref_counting.h>

namespace rtp {

struct RawKdNode : public vr::RefCounted
{
	typedef std::vector<uint32> Elements;

	RawKdNode();
	// Leaf node
	RawKdNode( const Elements& ids );
	// Internal node
	RawKdNode( const rt::SplitPlane& plane, RawKdNode* leftChild, RawKdNode* rightChild );

	inline bool isLeaf();

	rt::SplitPlane split;
	vr::ref_ptr<RawKdNode> left;
	vr::ref_ptr<RawKdNode> right;
	Elements elements;
};

inline bool RawKdNode::isLeaf()
{
	return ( left == NULL ) && ( right == NULL );
}

//////////////////////////////////////////////////////////////////////////

struct RawKdTree : public vr::RefCounted
{
	struct Statistics
	{
		Statistics();
		void reset();

		uint32 nodeCount;
		uint32 leafCount;
		uint32 treeDepth;
		uint32 elemIdCount;
		// TODO: other useful stats
	};

	vr::ref_ptr<RawKdNode> root;
	rt::Aabb bbox;
	Statistics stats;
};

} // namespace rtp

#endif // _RTP_RAWKDTREE_H_
