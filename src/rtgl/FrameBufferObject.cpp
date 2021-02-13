#include <rtgl/FrameBufferObject.h>

using namespace rtgl;

FrameBufferObject::~FrameBufferObject()
{
	destroy();
}

FrameBufferObject* FrameBufferObject::create()
{
	return new FrameBufferObject();
}

bool FrameBufferObject::init( Texture* texture, bool attachDepthBuffer )
{
	if( !GLEW_EXT_framebuffer_object )
		return false;

	// Delete previous data, if any
	destroy();

	// Create FBO
	glGenFramebuffersEXT( 1, &_bufferId );
	if( _bufferId == 0 )
		return false;

	// Save texture
	_tex = texture;

	bind();

	// Create depth buffer and attach to FBO
	if( attachDepthBuffer )
	{
		glGenRenderbuffersEXT( 1, &_depthId );
		glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, _depthId );
		glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, _tex->getWidth(), _tex->getHeight() );
		glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, _depthId );
		glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, 0 );
	}

	// Attach texture to FBO
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _tex->getTextureId(), 0 );

	// Check FBO
	GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
	if( status != GL_FRAMEBUFFER_COMPLETE_EXT )
		return false;

	// Release FBO
	release();

	return true;
}

GLuint FrameBufferObject::getBufferId() const
{
	return _bufferId;
}

void FrameBufferObject::setViewport( GLsizei width, GLsizei height )
{
	bind();
	glViewport( 0, 0, width, height );
	release();
}

void FrameBufferObject::bind() const
{
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _bufferId );
}

void FrameBufferObject::release() const
{
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
}

Texture* FrameBufferObject::getTexture()
{
	return _tex.get();
}

void FrameBufferObject::destroy()
{
	if( _depthId > 0 )
	{
		glDeleteRenderbuffersEXT( 1, &_depthId );
		glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, 0 );
	}

	if( _bufferId > 0 )
	{
		glDeleteFramebuffersEXT( 1, &_bufferId );
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	}
}

// Private
FrameBufferObject::FrameBufferObject()
{
	_bufferId = 0;
	_depthId = 0;
}
