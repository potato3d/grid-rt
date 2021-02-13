#ifndef _RTGL_TEXTURE_H_
#define _RTGL_TEXTURE_H_

#include <rtgl/common.h>
#include <vr/ref_counting.h>

namespace rtgl {

// TODO: filters and env modes
// Default behavior is of a 2D texture
class Texture : public vr::RefCounted
{
public:
	enum Target
	{
		Target_2D             = GL_TEXTURE_2D,
		Target_2D_Unormalized = GL_TEXTURE_RECTANGLE_ARB,
		Target_3D             = GL_TEXTURE_3D
	};

	enum Format
	{
		Format_Luminance_Ubyte,
		Format_Luminance_Float,

		Format_Luminance_Alpha_Ubyte,
		Format_Luminance_Alpha_Float,
		
		Format_BGR_Ubyte,
		Format_BGR_Float,	// cannot be used as internal format
		Format_BGRA_Ubyte,
		Format_BGRA_Float,	// cannot be used as internal format

		Format_RGB_Ubyte,
		Format_RGB_Float,
		Format_RGBA_Ubyte,
		Format_RGBA_Float
	};

	enum Filter
	{
		Filter_Nearest = GL_NEAREST,
		Filter_Linear  = GL_LINEAR
	};

	// Create an empty Texture object
	static Texture* create();

	// Destroys Texture in OpenGL
	~Texture();

	void setFilters( Filter minFilter, Filter magFilter );

	bool init( Format internal, Format external, Target target = Target_2D );

	// If data is NULL, only reallocates memory in GPU
	void setImage( void* data, GLsizei width, GLsizei height, GLsizei depth = 1 );

	// Data must have been allocated at least once, using setImage()
	void setSubImage( void* data, GLsizei width, GLsizei height, GLsizei depth = 1 );

	// Binds this texture to current texture unit
	void bind() const;

	// Unbinds this texture from current texture unit
	void release() const;

	GLuint getTextureId() const;
	Target getTarget() const;
	GLenum getInternalFormat() const;
	GLenum getExternalFormat() const;
	GLenum getExternalType() const;
	GLsizei getTexelSizeBytes() const;
	GLsizei getWidth() const;
	GLsizei getHeight() const;
	GLsizei getDepth() const;
	GLsizei getTotalSizeBytes() const;

	// Delete data from OpenGL
	void destroy();

private:
	// Force heap allocation
	// This allows 1:1 relationship between object life and texture life in OpenGL
	Texture();

	GLuint  _texId;
	Target  _target;
	GLenum  _internalFormat;
	GLenum  _externalFormat;
	GLenum  _externalType;
	GLsizei _texelSizeBytes;
	GLsizei _width;
	GLsizei _height;
	GLsizei _depth;

	Filter _minFilter;
	Filter _magFilter;
};

} // namespace rtgl

#endif // _RTGL_TEXTURE_H_
