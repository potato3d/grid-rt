#include <rtp/Texture2D.h>

using namespace rtp;

Texture2D::Texture2D()
{
	_texels = NULL;
}

void Texture2D::setFilter( RTenum type )
{
	_filter = type;
}

void Texture2D::setWrapS( RTenum type )
{
	_wrapS = type;
}

void Texture2D::setWrapT( RTenum type )
{
	_wrapT = type;
}

void Texture2D::setEnvMode( RTenum type )
{
	_envMode = type;
}

void Texture2D::setTextureImage2D( uint32 width, uint32 height, unsigned char* texels )
{
	_width = width;
	_height = height;
	if( _texels )
		delete [] _texels;

	_texels = new unsigned char[width*height*4]; // hard-coded RGBA format

	std::copy( texels, texels + width*height*4, _texels );
}

void Texture2D::shade( rt::Sample& sample )
{
	// Compute texture coordinates from interpolated vertex attributes.
	// Instead, we could use different texture mapping algorithms to automatically generate texture coordinates.
	// Ex: planar, sphere, cylinder, etc
	vr::vec3f coords;
	sample.computeTexCoords( coords );

	// Just to be safe
	// TODO: can be removed?
	if( ( coords.x < 0.0f ) || ( coords.y < 0.0f ) )
		return;

	// Compute correct texture coordinates given wrap modes
	switch( _wrapS )
	{
	case RT_REPEAT:
		coords.x = vr::frac( coords.x );
		break;

	case RT_CLAMP:
		coords.x = vr::max( vr::min( coords.x, 1.0f ), 0.0f );
		break;

	default:
		break;
	}

	switch( _wrapT )
	{
	case RT_REPEAT:
		coords.y = vr::frac( coords.y );
		break;

	case RT_CLAMP:
		coords.y = vr::max( vr::min( coords.y, 1.0f ), 0.0f );
		break;

	default:
		break;
	}
	
	// Final texel color
	vr::vec3f texel;

	float xIndexF = coords.x * ( _width - 1 );
	float yIndexF = coords.y * ( _height - 1 );

	// Compute texel color from correct texture coordinates
	switch( _filter )
	{
	case RT_NEAREST:
		{
			// TODO: optimize
			int32 s = (int32)vr::round( xIndexF );
			int32 t = (int32)vr::round( yIndexF );
			unsigned char* imgTexel = _texels + ( s + t*_width )*4;
			texel.set( (float)imgTexel[0]/255.0f, (float)imgTexel[1]/255.0f, (float)imgTexel[2]/255.0f );
			break;
		}

	case RT_LINEAR:
		{
			// TODO: optimize
			// Get 4 corner pixels and lerp between them
			int u0 = (int)xIndexF;
			int v0 = (int)yIndexF;
			int u1 = u0 + 1;
			int v1 = v0 + 1;

			float ds = vr::frac( xIndexF );
			float dt = vr::frac( yIndexF );

			unsigned char* lowerLeft  = _texels + ( u0 + v0*_width )*4;
			unsigned char* lowerRight = _texels + ( u1 + v0*_width )*4;
			unsigned char* upperLeft  = _texels + ( u0 + v1*_width )*4;
			unsigned char* upperRight = _texels + ( u1 + v1*_width )*4;

			vr::vec3f lowerLeftF (  (float)lowerLeft[0]/255.0f,  (float)lowerLeft[1]/255.0f,  (float)lowerLeft[2]/255.0f  );
			vr::vec3f lowerRightF( (float)lowerRight[0]/255.0f, (float)lowerRight[1]/255.0f, (float)lowerRight[2]/255.0f );
			vr::vec3f upperLeftF (  (float)upperLeft[0]/255.0f,  (float)upperLeft[1]/255.0f,  (float)upperLeft[2]/255.0f  );
			vr::vec3f upperRightF( (float)upperRight[0]/255.0f, (float)upperRight[1]/255.0f, (float)upperRight[2]/255.0f );

			vr::vec3f lowerInterp( lowerLeftF * ( 1.0f - ds ) + lowerRightF * ds );
			vr::vec3f upperInterp( upperLeftF * ( 1.0f - ds ) + upperRightF * ds );

			texel = lowerInterp * ( 1.0f - dt ) + upperInterp * dt;
			break;
		}

	default:
		break;
	}

	// Finally, apply texture color to given sample color
	switch( _envMode )
	{
	case RT_MODULATE:
		sample.color *= texel;
		break;

	case RT_REPLACE:
		sample.color = texel;
		break;

	default:
		break;
	}
}
