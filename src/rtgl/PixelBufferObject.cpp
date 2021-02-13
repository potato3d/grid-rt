#include <rtgl/PixelBufferObject.h>
#include <iostream>

using namespace rtgl;

PixelBufferObject* PixelBufferObject::create()
{
	return new PixelBufferObject();
}

PixelBufferObject::~PixelBufferObject()
{
	destroy();
}

bool PixelBufferObject::init()
{
	// Check extension support
	if( !GLEW_ARB_pixel_buffer_object )
		return false;

	// Delete previous data, if any
	destroy();

	// Create PBO
	glGenBuffers( 1, &_bufferId );
	if( _bufferId == 0 )
		return false;

	return true;
}

GLuint PixelBufferObject::getBufferId() const
{
	return _bufferId;
}

void PixelBufferObject::setTexture( Texture* texture )
{
	if( texture->getTarget() == Texture::Target_3D )
	{
		printf( "Error (PixelBufferObject::setTexture): Unsupported texture target.\n" );
		return;
	}
	_tex = texture;
}

void PixelBufferObject::reallocate( GLenum usageHint, GLsizei width, GLsizei height )
{
	switch( usageHint )
	{
	case GL_STREAM_DRAW:
	case GL_DYNAMIC_DRAW:
	case GL_STATIC_DRAW:
	case GL_STREAM_COPY:
	case GL_DYNAMIC_COPY:
	case GL_STATIC_COPY:
		_targetType = GL_PIXEL_UNPACK_BUFFER;
		break;

	case GL_STREAM_READ:
	case GL_DYNAMIC_READ:
	case GL_STATIC_READ:
		_targetType = GL_PIXEL_PACK_BUFFER;
		break;

	default:
	    return;
	}

 	bind();

	// Reallocate buffer
	glBufferData( _targetType, width*height*_tex->getTexelSizeBytes(), NULL, usageHint );

	// Reallocate texture
	if( _tex.valid() )
		_tex->setImage( NULL, width, height );

	release();
}

void PixelBufferObject::bind() const
{
	if( _tex.valid() )
		_tex->bind();
	glBindBuffer( _targetType, _bufferId );
}

const void* PixelBufferObject::beginRead() const
{
	glReadPixels( 0, 0, _tex->getWidth(), _tex->getHeight(), 
		          _tex->getExternalFormat(), _tex->getExternalType(), NULL );
	return glMapBuffer( _targetType, GL_READ_ONLY );
}

void PixelBufferObject::endRead() const
{
	glUnmapBuffer( _targetType );
}

void* PixelBufferObject::beginWrite() const
{
	return glMapBuffer( _targetType, GL_WRITE_ONLY );
}

void PixelBufferObject::endWrite() const
{
	glUnmapBuffer( _targetType );
	glTexSubImage2D( _tex->getTarget(), 0, 0, 0, _tex->getWidth(), _tex->getHeight(),
		             _tex->getExternalFormat(), _tex->getExternalType(), NULL );
}

void PixelBufferObject::release() const
{
	glBindBuffer( _targetType, 0 );
	if( _tex.valid() )
		_tex->release();
}

Texture* PixelBufferObject::getTexture()
{
	return _tex.get();
}

void PixelBufferObject::destroy()
{
	if( _bufferId > 0 )
		glDeleteBuffers( 1, &_bufferId );
}

// Private
PixelBufferObject::PixelBufferObject()
{
	_bufferId = 0;
	_targetType = 0;
}
