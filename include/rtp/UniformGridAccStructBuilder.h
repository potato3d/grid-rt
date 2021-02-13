#ifndef _RTP_UNIFORMGRIDACCSTRUCTBUILDER_H_
#define _RTP_UNIFORMGRIDACCSTRUCTBUILDER_H_

#include <rt/IAccStructBuilder.h>

namespace rtp {

class UniformGridAccStructBuilder : public rt::IAccStructBuilder
{
public:
	virtual void buildGeometry( rt::Geometry* geometry );
	// TODO: 
	//virtual IAccStruct* buildInstance( const std::vector<rt::Instance> instances );

private:
	void buildUniformGrid( rt::Geometry* geometry );
	void buildCubeGrid( rt::Geometry* geometry );
};

} // namespace rtp

#endif // _RTP_UNIFORMGRIDACCSTRUCTBUILDER_H_
