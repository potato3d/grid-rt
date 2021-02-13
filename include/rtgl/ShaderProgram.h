#ifndef _RTGL_SHADERPROGRAM_H_
#define _RTGL_SHADERPROGRAM_H_

#include <rtgl/common.h>

namespace rtgl {

class ShaderProgram
{
public:
	ShaderProgram();
	~ShaderProgram();

	void setFragmentFile( const vr::String& filename );
	const vr::String& getFragmentFile() const;

	void setVertexFile( const vr::String& filename );
	const vr::String& getVertexFile() const;

	void setGeometryFile( const vr::String& filename );
	const vr::String& getGeometryFile() const;

	// TODO: geometry shader specific parameters

	bool create();
	bool reloadShaders();

	void bind();

	void setUniform( const vr::String& symbolName, GLint value );
	void setUniform( const vr::String& symbolName, GLfloat value );
	void setUniform( const vr::String& symbolName, const GLfloat value[3], GLsizei count = 1 );
	void setUniform( const vr::String& symbolName, const GLint* value, GLsizei count );

	void release();

	void destroyShaders();
	void destroy();

private:
	bool loadShaderFile( GLhandleARB programObject, const vr::String& filename, GLuint type );

	GLuint _programId;

	vr::String _fragFile;
	vr::String _vertFile;
	vr::String _geomFile;
};

} // namespace rtgl

#endif // _RTGL_SHADERPROGRAM_H_
