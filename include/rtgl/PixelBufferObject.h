#ifndef _RTGL_PIXELBUFFEROBJECT_H_
#define _RTGL_PIXELBUFFEROBJECT_H_

#include <rtgl/common.h>
#include <rtgl/Texture.h>
#include <vr/ref_counting.h>

namespace rtgl {

// TODO: we do not actually need to bind the texture every time, only when write pbo data to it (endWrite)
class PixelBufferObject : public vr::RefCounted
{
public:
	// Create an empty Pixel Buffer Object
	static PixelBufferObject* create();

	~PixelBufferObject();

	// Create pixel buffer, still needs a texture to read/writes
	bool init();

	// Get OpenGL handle
	GLuint getBufferId() const;

	// Set previously created texture to be used with PBO
	// PBO takes ownership of texture (i.e. controls binds and size)
	void setTexture( Texture* texture );

	// Reallocate buffer storage in GPU
	// Also reallocates texture
	void reallocate( GLenum usageHint, GLsizei width, GLsizei height );

	// Bind buffer in order to execute read/write operations
	// Binds attached texture
	void bind() const;

	// Read from GPU to CPU, and map buffer to access data
	const void* beginRead() const;

	// Unmap buffer
	void endRead() const;

	// Map buffer so that CPU can change it
	void* beginWrite() const;

	// Unmap buffer and transfer data from CPU to GPU
	void endWrite() const;

	// Always release buffer when you're done to avoid side effects
	// Releases attached texture
	void release() const;

	// Get texture associated with PBO
	Texture* getTexture();

	// Delete buffer
	// Keeps texture intact
	void destroy();

private:
	// Force heap allocation
	// This allows 1:1 relationship between object life and buffer life in OpenGL
	PixelBufferObject();

	GLuint  _bufferId;
	GLenum  _targetType;
	vr::ref_ptr<Texture> _tex;
};

} // namespace rtgl

#endif // _RTGL_PIXELBUFFEROBJECT_H_
