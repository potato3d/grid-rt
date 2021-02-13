#ifndef _RTP_KDTREEACCSTRUCT_H_
#define _RTP_KDTREEACCSTRUCT_H_

#include <rt/IAccStruct.h>
#include <rt/SplitPlane.h>
#include <rt/Stack.h>

namespace rtp {

class KdNode
{
public:
	void setInternalNode( const rt::SplitPlane& plane, const KdNode* leftChild );
	void setLeafNode( uint32 elementStart, uint32 elementCount );

	inline uint32 isLeaf() const;
	inline uint32 axis() const;
	inline float splitPos() const;
	inline const KdNode* leftChild() const;
	inline uint32 elemStart() const;
	inline uint32 elemCount() const;

private:
	//--- If internal node ---
	// bits 0..1 : split axis
	// bits 2..30 : pointer to left child
	// bit 31 (sign) : flag whether node is a leaf
	//--- If leaf node ---
	// bits 0..30 : number of elements stored in leaf
	// bit 31 (sign) : flag whether node is a leaf
	uint32 _data;

	//--- If internal node ---
	// split position along specified axis
	//--- If leaf node ---
	// offset to start of elements
	union
	{
		float _split;
		uint32 _elements;
	};
};

inline uint32 KdNode::isLeaf() const
{
	return ( _data & 0x80000000 );
}

inline uint32 KdNode::axis() const
{
	return ( _data & 0x3 );
}

inline float KdNode::splitPos() const
{
	return _split;
}

inline const KdNode* KdNode::leftChild() const
{
#pragma warning( disable : 4312 )
	return reinterpret_cast<const KdNode*>( _data & 0x7FFFFFFC );
#pragma warning( default : 4312 )
}

inline uint32 KdNode::elemStart() const
{
	return _elements;
}

inline uint32 KdNode::elemCount() const
{
	return ( _data & 0x7FFFFFFF );
}

//////////////////////////////////////////////////////////////////////////

class KdTreeAccStruct : public rt::IAccStruct
{
public:
	static const unsigned int MAX_STACK_SIZE = 128;

	// Traversal stack information
	struct TraversalData
	{
		const KdNode* node;
		float tnear;
		float tfar;
		int32 pad;
	};

	typedef rt::Stack<TraversalData, MAX_STACK_SIZE> TraversalStack;

	KdTreeAccStruct();
	// Cannot make destructor to avoid accidental deletion of data.
	// Example: resize in vector<geometry> will _shallow_ copy two geometries, and their kdtree.
	// One of them will be destroyed, thus deleting the other's data.
	// Should make a deep copy operator instead

	virtual void clear();

	virtual void traceNearestInstance( const std::vector<rt::Instance>& instances, rt::Sample& sample );
	virtual void traceNearestGeometry( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit );

	KdNode* root;
	uint32* elements;

private:
	void findLeaf( const KdNode*& node, rt::Ray& ray, TraversalStack& stack );
};

} // namespace rtp

#endif // _RTP_KDTREEACCSTRUCT_H_
