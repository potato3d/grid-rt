#ifndef _RTP_HEADLIGHTMATERIAL_H_
#define _RTP_HEADLIGHTMATERIAL_H_

#include <rt/IMaterial.h>

namespace rtp {

class HeadlightMaterial : public rt::IMaterial
{
public:
	HeadlightMaterial();

	virtual void shade( rt::Sample& sample );

private:
	vr::vec3f _ambient;
};

} // namespace rtp

#endif // _RTP_HEADLIGHTMATERIAL_H_
