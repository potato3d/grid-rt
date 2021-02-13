#ifndef _RTP_KDTREEACCSTRUCTBUILDER_H_
#define _RTP_KDTREEACCSTRUCTBUILDER_H_

#include <rt/IAccStructBuilder.h>

namespace rtp {

// Forward declarations
struct RawKdTree;
class KdTreeAccStruct;
class TriangleTreeBuilder;
class InstanceTreeBuilder;

class KdTreeAccStructBuilder : public rt::IAccStructBuilder
{
public:
	KdTreeAccStructBuilder();
	~KdTreeAccStructBuilder();

	virtual void buildGeometry( rt::Geometry* geometry );
	virtual rt::IAccStruct* buildInstance( const std::vector<rt::Instance> instances );

private:
	KdTreeAccStruct* convertRawTree( RawKdTree* tree );

	TriangleTreeBuilder* _triangleTreeBuilder;
	InstanceTreeBuilder* _instanceTreeBuilder;
};

} // namespace rtp

#endif // _RTP_KDTREEACCSTRUCTBUILDER_H_
