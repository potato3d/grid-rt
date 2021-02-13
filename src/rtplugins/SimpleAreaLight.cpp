#include <rtp/SimpleAreaLight.h>
#include <rt/Context.h>
#include <vr/random.h>

using namespace rtp;

SimpleAreaLight::SimpleAreaLight()
: SimplePointLight()
{
	_radius = 1.0f;
	_sampleCount = 4;
}

bool SimpleAreaLight::illuminate( rt::Sample& sample )
{
	// Avoid back face lighting
	if( !sample.isFrontFace() )
		return false;

	// Direction towards light
	const vr::vec3f L = _position - sample.hitPosition;

	// Assume a disk perpendicular to direction using _radius and compute _sampleCount samples randomly inside it
	vr::vec3f uAxis;
	vr::vec3f vAxis;
	vr::vec3f samplePos;
	vr::vec3f dir = L;
	dir.normalize();
	dir.orthonormalBasis( uAxis, vAxis );
	float x;
	float y;
	uint32 successfulSamples = 0;

	rt::Context* ctx = rt::Context::current();

	for( uint32 i = 0; i < _sampleCount; ++i )
	{
		randomDisk( x, y );
		x *= _radius;
		y *= _radius;
		samplePos = L + ( uAxis * x ) + ( vAxis * y );

		// Setup shadow ray
		// Since we did not normalize the direction, every parametric t step walks the length of the direction along the ray.
		// So, when t == 1 we are right at the light position, which is the farthest we want to go.
		const bool ok = sample.initShadowRay( samplePos - sample.hitPosition, 1.0f );

		// Avoid computing light contribution for triangles facing away
		if( !ok )
			continue;

		// If light sample is occluded, we avoid computing its contribution
		if( _castShadows && ctx->traceAny( sample ) )
			continue;

		++successfulSamples;
	}

	// If no samples hit light
	if( successfulSamples == 0 )
		return false;

	// Quadratic distance attenuation
	const float distance = L.length(); // TODO: if it's slow, we can use only squared distance and attenuation
	const float attenFactor = vr::max( 1.0f, 1.0f / ( _constAtten + _linearAtten * distance + _quadAtten * distance * distance ) );

	// Compute and return light intensity
	sample.color = _intensity * attenFactor * ( (float)successfulSamples / (float)_sampleCount );

	// Store original direction to light for shading computations
	sample.ray.dir = L;

	return true;
}

void SimpleAreaLight::randomDisk( float& x, float& y )
{
	const float r = sqrtf( vr::Random::realInIn() );
	const float theta = vr::Mathf::TWO_PI * vr::Random::realInIn();

	x = r*cosf( theta );
	y = r*sinf( theta );
}
