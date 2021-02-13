#include <rtp/PhongMaterial.h>
#include <rt/Context.h>
#include <rt/ILight.h>

using namespace rtp;

PhongMaterial::PhongMaterial()
{
	_ambient.set( 0.2f, 0.2f, 0.2f );
	_specularColor.set( 1.0f, 1.0f, 1.0f );
	_specularExponent = 32.0f;
	_reflexCoeff = 0.0f;
	_refractionIndex = 1.0f;
	_opacity = 1.0f;
	_texture = NULL;
}

void PhongMaterial::shade( rt::Sample& sample )
{
	// Get data from current sample
	sample.computeShadingNormal();
	sample.computeHitPosition();
	
	vr::vec3f specularVector;
	sample.computeSpecularVector( specularVector );

	sample.computeShadingColor();
	vr::vec3f objColor( sample.color );

	// Query light sources
	rt::Context* ctx = rt::Context::current();
	const uint32 lightCount = ctx->getLightCount();

	// Accumulate light contributions
	vr::vec3f lightDiffuse( 0.0f, 0.0f, 0.0f );
	vr::vec3f specular( 0.0f, 0.0f, 0.0f );

	// Create new sample for light samples
	rt::Sample lightSample;

	for( uint32 i = 0; i < lightCount; ++i )
	{
		// Setup light sample
		lightSample.initLightRay( sample );

		// Update lightSample with light radiance and direction
		bool ok = ctx->getLight( i )->illuminate( lightSample );

		// Probably light is occluded or points away from hit point
		if( !ok )
			continue;

		// Query light sample direction and radiance
		lightSample.ray.dir.normalize();

		// Diffuse shading
		const float nDotL = sample.normal.dot( lightSample.ray.dir );
		lightDiffuse += lightSample.color * nDotL;

		// Specular shading
		const float specDotL = specularVector.dot( lightSample.ray.dir );
		if( specDotL > 0.0f )
			specular += lightSample.color * powf( specDotL, _specularExponent );
	}

	// Gather all lighting contributions
	sample.color = objColor * ( _ambient + lightDiffuse ) + ( _specularColor * specular );

	// Apply texture
	if( _texture != NULL )
		_texture->shade( sample );

	// Avoid infinite recursion
	if( sample.stopRayRecursion() )
		return;

	// Compute reflection contribution
	if( _reflexCoeff > 0.0f )
	{
		rt::Sample secondarySample;
		secondarySample.initReflectionRay( sample );

		// Trace reflection ray and add contribution
		ctx->traceNearest( secondarySample );
		sample.color += secondarySample.color * _reflexCoeff;
	}

	// Compute refraction contribution
	if( _opacity < 1.0f )
	{
		rt::Sample refractionSample;
		const bool haveRefraction = refractionSample.initRefractionRay( sample, _refractionIndex );

		// Check for total internal reflection
		if( haveRefraction )
		{
			// Trace refraction ray and add contribution
			ctx->traceNearest( refractionSample );
			sample.color += refractionSample.color * ( 1.0f - _opacity );
		}
	}
}

void PhongMaterial::setAmbient( float r, float g, float b )
{
	_ambient.set( r, g, b );
}

void PhongMaterial::setSpecularExponent( float expn )
{
	_specularExponent = expn;
}

void PhongMaterial::setReflexCoeff( float coeff )
{
	_reflexCoeff = coeff;
}

void PhongMaterial::setOpacity( float opacity )
{
	_opacity = opacity;
}

void PhongMaterial::setRefractionIndex( float index )
{
	_refractionIndex = index;
}

void PhongMaterial::setTexture( rt::ITexture* texture )
{
	_texture = texture;
}
