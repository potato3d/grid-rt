#include <rtp/SimplePointLight.h>
#include <rt/Context.h>

using namespace rtp;

SimplePointLight::SimplePointLight()
{
	_castShadows = true;
	_intensity.set( 1.0f, 1.0f, 1.0f );
	_position.set( 0.0f, 0.0f, 0.0f );

	_useDistanceAttenuation = false;
	_constAtten = 0.0f;
	_linearAtten = 0.005f;
	_quadAtten = 0.01f;
}

bool SimplePointLight::illuminate( rt::Sample& sample )
{
	// Avoid back face lighting
	if( !sample.isFrontFace() )
		return false;

	// Direction towards light
	const vr::vec3f L = _position - sample.hitPosition;

	// Setup shadow ray
	// Since we did not normalize the direction, every parametric t step walks the length of the direction along the ray.
	// So, when t == 1 we are right at the light position, which is the farthest we want to go.
	const bool ok = sample.initShadowRay( L, 1.0f );

	// Avoid computing light contribution for triangles facing away
	if( !ok )
		return false;

	// If light is occluded, we avoid computing its contribution
	if( _castShadows && rt::Context::current()->traceAny( sample ) )
		return false;

	float attenFactor = 1.0f;

	// Quadratic distance attenuation
	if( _useDistanceAttenuation )
	{
		const float distance = L.length(); // TODO: if it's slow, we can use only squared distance and attenuation
		attenFactor = vr::max( 1.0f, 1.0f / ( _constAtten + _linearAtten * distance + _quadAtten * distance * distance ) );
	}

	// Compute and return light intensity
	sample.color = _intensity * attenFactor;
	return true;
}

void SimplePointLight::setCastShadows( bool enabled )
{
	_castShadows = enabled;
}

void SimplePointLight::setIntensity( float x, float y, float z )
{
	_intensity.set( x, y, z );
}

void SimplePointLight::setPosition( float x, float y, float z )
{
	_position.set( x, y, z );
}

void SimplePointLight::setConstantAttenuation( float atten )
{
	_constAtten = atten;
}

void SimplePointLight::setLinearAttenuation( float atten )
{
	_linearAtten = atten;
}

void SimplePointLight::setQuadraticAttenuation( float atten )
{
	_quadAtten = atten;
}
