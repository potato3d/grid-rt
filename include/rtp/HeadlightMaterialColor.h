#ifndef _RTP_HEADLIGHTMATERIALCOLOR_H_
#define _RTP_HEADLIGHTMATERIALCOLOR_H_

#include <rt/IMaterial.h>

namespace rtp {

class HeadlightMaterialColor : public rt::IMaterial
{
public:
	HeadlightMaterialColor();

	virtual void shade( rt::Sample& sample );

	void setAmbient( float r, float g, float b );
	void setDiffuse( float r, float g, float b );

private:
	vr::vec3f _ambient;
	vr::vec3f _diffuse;
};

} // namespace rtp

#endif // _RTP_HEADLIGHTMATERIALCOLOR_H_
