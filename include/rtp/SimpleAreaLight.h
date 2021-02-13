#ifndef _RTP_SIMPLEAREALIGHT_H_
#define _RTP_SIMPLEAREALIGHT_H_

#include <rtp/SimplePointLight.h>

namespace rtp {

class SimpleAreaLight : public SimplePointLight
{
public:
	SimpleAreaLight();

	virtual bool illuminate( rt::Sample& sample );

private:
	void randomDisk( float& x, float& y );

	float _radius;
	float _area;
	uint32 _sampleCount;
};

} // namespace rtp

#endif // _RTP_SIMPLEAREALIGHT_H_
