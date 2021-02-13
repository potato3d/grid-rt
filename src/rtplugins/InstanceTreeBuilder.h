#ifndef _RTP_INSTANCETREEBUILDER_H_
#define _RTP_INSTANCETREEBUILDER_H_

#include <rt/Instance.h>
#include "RawKdTree.h"

namespace rtp {

class InstanceTreeBuilder
{
public:
	RawKdTree* buildTree( const std::vector<rt::Instance>& instances );

private:
	RawKdNode* recursiveBuild( const RawKdNode::Elements& instances, const rt::Aabb& bbox, uint32 treeDepth );

	// Find axis in descending order of maximum extent
	void orderAxis( uint32* axis, const rt::Aabb& bbox );
	RawKdNode* leafNode( const RawKdNode::Elements& instances, uint32 treeDepth );

	const std::vector<rt::Instance>* _instances;
	RawKdTree::Statistics* _stats;
};

} // namespace rtp

#endif // _RTP_INSTANCETREEBUILDER_H_
