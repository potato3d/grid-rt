#ifndef _RTP_PHONGMATERIALCOLOR_H_
#define _RTP_PHONGMATERIALCOLOR_H_

#include <rt/IMaterial.h>
#include <rt/ITexture.h>

namespace rtp {

class PhongMaterialColor : public rt::IMaterial
{
public:
	PhongMaterialColor();

	virtual void shade( rt::Sample& sample );

	void setAmbient( float r, float g, float b );
	void setDiffuse( float r, float g, float b );
	void setSpecular( float r, float g, float b );
	void setSpecularExponent( float expn );
	void setReflexCoeff( float coeff );
	void setRefractionIndex( float index );
	void setOpacity( float opacity );
	void setTexture( uint32 texId );

//private:
	vr::vec3f _ambient;
	vr::vec3f _diffuse;
	vr::vec3f _specularColor;
	float _specularExponent;
	float _reflexCoeff;
	float _refractionIndex;
	float _opacity;
	uint32 _textureId;
};

} // namespace rtp

#endif // _RTP_PHONGMATERIALCOLOR_H_
