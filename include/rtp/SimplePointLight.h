#ifndef _RTP_SIMPLEPOINTLIGHT_H_
#define _RTP_SIMPLEPOINTLIGHT_H_

#include <rt/ILight.h>

namespace rtp {

class SimplePointLight : public rt::ILight
{
public:
	SimplePointLight();

	virtual bool illuminate( rt::Sample& sample );

	void setCastShadows( bool enabled );
	void setIntensity( float x, float y, float z );
	void setPosition( float x, float y, float z );

	void setUseDistanceAttenuation( bool use );
	void setConstantAttenuation( float atten );
	void setLinearAttenuation( float atten );
	void setQuadraticAttenuation( float atten );

//protected:
	bool _castShadows;
	vr::vec3f _intensity;
	vr::vec3f _position;

	bool _useDistanceAttenuation;
	float _constAtten;
	float _linearAtten;
	float _quadAtten;
};

} // namespace rtp

#endif // _RTP_SIMPLEPOINTLIGHT_H_
