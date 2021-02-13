#include <rtgl/Texture.h>
#include <iostream>

using namespace rtgl;

Texture* Texture::create()
{
	return new Texture();
}

Texture::~Texture()
{
	destroy();
}

void Texture::setFilters( Filter minFilter, Filter magFilter )
{
	_minFilter = minFilter;
	_magFilter = magFilter;
}

bool Texture::init( Format internal, Format external, Target target )
{
	// Delete previous data, if any
	destroy();

	// Store OpenGL parameters
	switch( internal )
	{
	case Format_Luminance_Ubyte:
		_internalFormat = GL_LUMINANCE8;
		break;

	case Format_Luminance_Float:
		_internalFormat = GL_LUMINANCE32F_ARB;
		break;

	case Format_Luminance_Alpha_Ubyte:
		_internalFormat = GL_LUMINANCE_ALPHA;
		break;

	case Format_Luminance_Alpha_Float:
		_internalFormat = GL_LUMINANCE_ALPHA32F_ARB;
		break;

	case Format_BGR_Ubyte:
		_internalFormat = GL_BGR;
		break;

	case Format_BGRA_Ubyte:
		_internalFormat = GL_BGRA;
		break;

	case Format_RGB_Ubyte:
		_internalFormat = GL_RGB8;
		break;

	case Format_RGB_Float:
		_internalFormat = GL_RGB32F_ARB;
		break;

	case Format_RGBA_Ubyte:
		_internalFormat = GL_RGBA8;
		break;

	case Format_RGBA_Float:
		_internalFormat = GL_RGBA32F_ARB;
		break;

	default:
		printf( "Error (Texture2D::init): Unrecognized internal format.\n" );
		return false;
	}

	// Store OpenGL parameters
	switch( external )
	{
	case Format_Luminance_Ubyte:
		_externalFormat = GL_LUMINANCE;
		_externalType = GL_UNSIGNED_BYTE;
		_texelSizeBytes = 1*sizeof(unsigned char);
		break;

	case Format_Luminance_Float:
		_externalFormat = GL_LUMINANCE;
		_externalType = GL_FLOAT;
		_texelSizeBytes = 1*sizeof(float);
		break;

	case Format_Luminance_Alpha_Ubyte:
		_externalFormat = GL_LUMINANCE_ALPHA;
		_externalType = GL_UNSIGNED_BYTE;
		_texelSizeBytes = 2*sizeof(unsigned char);
		break;

	case Format_Luminance_Alpha_Float:
		_externalFormat = GL_LUMINANCE_ALPHA;
		_externalType = GL_FLOAT;
		_texelSizeBytes = 2*sizeof(float);
		break;

	case Format_BGR_Ubyte:
		_externalFormat = GL_BGR;
		_externalType = GL_UNSIGNED_BYTE;
		_texelSizeBytes = 3*sizeof(unsigned char);
		break;

	case Format_BGR_Float:
		_externalFormat = GL_BGR;
		_externalType = GL_FLOAT;
		_texelSizeBytes = 3*sizeof(float);
		break;

	case Format_BGRA_Ubyte:
		_externalFormat = GL_BGRA;
		_externalType = GL_UNSIGNED_BYTE;
		_texelSizeBytes = 4*sizeof(unsigned char);
		break;

	case Format_BGRA_Float:
		_externalFormat = GL_BGRA;
		_externalType = GL_FLOAT;
		_texelSizeBytes = 4*sizeof(float);
		break;

	case Format_RGB_Ubyte:
		_externalFormat = GL_RGB;
		_externalType = GL_UNSIGNED_BYTE;
		_texelSizeBytes = 3*sizeof(unsigned char);
		break;

	case Format_RGB_Float:
		_externalFormat = GL_RGB;
		_externalType = GL_FLOAT;
		_texelSizeBytes = 3*sizeof(float);
		break;

	case Format_RGBA_Ubyte:
		_externalFormat = GL_RGBA;
		_externalType = GL_UNSIGNED_BYTE;
		_texelSizeBytes = 4*sizeof(unsigned char);
		break;

	case Format_RGBA_Float:
		_externalFormat = GL_RGBA;
		_externalType = GL_FLOAT;
		_texelSizeBytes = 4*sizeof(float);
		break;

	default:
		printf( "Error (Texture2D::init): Unrecognized external format.\n" );
		return false;
	}

	_target = target;

	glGenTextures( 1, &_texId );
	if( _texId == 0 )
		return false;

	bind();
	glTexParameteri( _target, GL_TEXTURE_MIN_FILTER, _minFilter );
	glTexParameteri( _target, GL_TEXTURE_MAG_FILTER, _magFilter );
	glTexParameteri( _target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( _target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( _target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	release();

	return true;
}

void Texture::setImage( void* data, GLsizei width, GLsizei height, GLsizei depth )
{
	_width = width;
	_height = height;
	_depth = depth;

	bind();

	if( _target != Target_3D )
		glTexImage2D( _target, 0, _internalFormat, _width, _height, 0, _externalFormat, _externalType, data );
	else
		glTexImage3D( _target, 0, _internalFormat, _width, _height, _depth, 0, _externalFormat, _externalType, data );

	release();
}

void Texture::setSubImage( void* data, GLsizei width, GLsizei height, GLsizei depth )
{
	bind();

	if( _target != Target_3D )
		glTexSubImage2D( _target, 0, 0, 0, _width, _height, _externalFormat, _externalType, data );
	else
		glTexSubImage3D( _target, 0, 0, 0, 0, _width, _height, _depth, _externalFormat, _externalType, data );

	release();
}

void Texture::bind() const
{
	glBindTexture( _target, _texId );
}

void Texture::release() const
{
	glBindTexture( _target, 0 );
}

GLuint Texture::getTextureId() const
{
	return _texId;
}

Texture::Target Texture::getTarget() const
{
	return _target;
}

GLenum Texture::getInternalFormat() const
{
	return _internalFormat;
}

GLenum Texture::getExternalFormat() const
{
	return _externalFormat;
}

GLenum Texture::getExternalType() const
{
	return _externalType;
}

GLsizei Texture::getTexelSizeBytes() const
{
	return _texelSizeBytes;
}

GLsizei Texture::getWidth() const
{
	return _width;
}

GLsizei Texture::getHeight() const
{
	return _height;
}

GLsizei Texture::getDepth() const
{
	return _depth;
}

GLsizei Texture::getTotalSizeBytes() const
{
	return getWidth() * getHeight() * getDepth() * getTexelSizeBytes();
}

void Texture::destroy()
{
	if( _texId > 0 )
		glDeleteTextures( 1, &_texId );
}

// Private
Texture::Texture()
{
	_texId = 0;
	_target = Target_2D;
	_internalFormat = 0;
	_externalFormat = 0;
	_externalType = 0;
	_texelSizeBytes = 0;
	_width = 0;
	_height = 0;
	_depth = 0;
	_minFilter = Filter_Nearest;
	_magFilter = Filter_Nearest;
}
