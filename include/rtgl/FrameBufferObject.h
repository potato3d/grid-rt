#ifndef _RTGL_FRAMEBUFFEROBJECT_H_
#define _RTGL_FRAMEBUFFEROBJECT_H_

#include <rtgl/common.h>
#include <rtgl/Texture.h>
#include <vr/ref_counting.h>

namespace rtgl {

class FrameBufferObject : public vr::RefCounted
{
public:
	// Create an empty Frame Buffer Object
	static FrameBufferObject* create();

	~FrameBufferObject();

	// Create FBO with texture as color buffer and optional depth buffer attached
	// Buffer size is determined by texture size
	// To resize FBO, call init with a different sized texture
	// Texture managing must be done from the outside (i.e. bind texture before using FBO)
	bool init( Texture* texture, bool attachDepthBuffer = false );

	// Get OpenGL handle
	GLuint getBufferId() const;

	// Sets glViewport for this Frame Buffer Object
	void setViewport( GLsizei width, GLsizei height );

	// Make this the render target
	// Need binded texture
	void bind() const;

	// Return to normal window system
	void release() const;

	// Get texture attached to FBO
	Texture* getTexture();

	// Delete FBO and attachments
	void destroy();

private:
	// Force heap allocation
	// This allows 1:1 relationship between object life and buffer life in OpenGL
	FrameBufferObject();

	GLuint _bufferId;
	GLuint _depthId;
	vr::ref_ptr<Texture> _tex;
};

} // namespace rtgl

#endif // _RTGL_FRAMEBUFFEROBJECT_H_
