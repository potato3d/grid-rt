#ifndef _RTP_TRIANGLETREEBUILDER_H_
#define _RTP_TRIANGLETREEBUILDER_H_

#include <rt/Geometry.h>
#include "RawKdTree.h"

namespace rtp {

class TriangleTreeBuilder
{
public:
	TriangleTreeBuilder();

	RawKdTree* buildTree( rt::Geometry* geometry );

private:
	enum Side
	{
		LEFT,
		RIGHT,
		BOTH,
		INVALID // triangle outside box
	};

	struct SahResult
	{
		float cost;
		Side side;
	};

	struct Event
	{
		enum Type
		{
			END,
			PLANAR,
			START
		};

		uint32 triangleId;
		float position;
		Type type;
	};

	struct EventOrder
	{
		inline bool operator()( const Event& first, const Event& second ) const;
	};

	RawKdNode* leafNode( const RawKdNode::Elements& triangles, uint32 treeDepth );
	void getTriangleVertices( vr::vec3f& v0, vr::vec3f& v1, vr::vec3f& v2, uint32 triangleId ) const;
	bool terminate( const SahResult& bestResult, uint32 triangleCount ) const;
	void sah( SahResult& result, const rt::SplitPlane& plane, const rt::Aabb& bbox, uint32 nL, uint32 nP, uint32 nR );

	// O( n log^2 n ) implementation
	RawKdNode* recursiveBuild( const RawKdNode::Elements& triangles, const rt::Aabb& bbox, uint32 treeDepth );
	void findPlane( rt::SplitPlane& plane, SahResult& sahResult, uint32& triangleCount, 
		            const RawKdNode::Elements& triangles, const rt::Aabb& bbox );
	void partition( RawKdNode::Elements& left, RawKdNode::Elements& right, const SahResult& sahResult, 
		            const rt::SplitPlane& plane, const RawKdNode::Elements& triangles );

	rt::Geometry* _geometry;
	RawKdTree::Statistics* _stats;

	std::vector< std::vector<Event> > _events;
	std::vector<Side> _triangleSides;
	std::vector<rt::Aabb> _triangleBoxes;

	float _traversalCost;
	float _intersectionCost;
};

inline bool TriangleTreeBuilder::EventOrder::operator()( const TriangleTreeBuilder::Event& first, 
	                                                     const TriangleTreeBuilder::Event& second ) const
{
	return ( first.position < second.position ) || ( ( first.position == second.position ) && ( first.type < second.type ) );
}

} // namespace rtp

#endif // _RTP_TRIANGLETREEBUILDER_H_
