#include <rtgl/ShaderProgram.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace rtgl;

ShaderProgram::ShaderProgram()
{
	_programId = 0;
}

ShaderProgram::~ShaderProgram()
{
	destroy();
}

void ShaderProgram::setFragmentFile( const vr::String& filename )
{
	_fragFile = filename;
}

const vr::String& ShaderProgram::getFragmentFile() const
{
	return _fragFile;
}

void ShaderProgram::setVertexFile( const vr::String& filename )
{
	_vertFile = filename;
}

const vr::String& ShaderProgram::getVertexFile() const
{
	return _vertFile;
}

bool ShaderProgram::create()
{
	// TODO: check geometry shader somewhere
	if( !GLEW_ARB_vertex_program || !GLEW_ARB_fragment_program )
		return false;

	// Delete previous data, if any
	destroy();

	_programId = glCreateProgram();
	return true;
}

bool ShaderProgram::reloadShaders()
{
	if( _fragFile.isEmpty() && _vertFile.isEmpty() && _geomFile.isEmpty() )
		return true;

	destroyShaders();
	bool shaderOk = true;
	std::cout << std::endl << "***** Loading shaders... *****" << std::endl << std::endl;

	// Fragment Shader to be used
	if( !_fragFile.isEmpty() )
		shaderOk &= loadShaderFile( _programId, _fragFile, GL_FRAGMENT_SHADER );

	// Vertex Shader to be used
	if( !_vertFile.isEmpty() )
		shaderOk &= loadShaderFile( _programId, _vertFile, GL_VERTEX_SHADER );

	// Geometry Shader to be used
	//if( !_geomFile.empty() )
	//{
	//	shaderOk &= loadShaderFile( _programId, _geomFile, GL_GEOMETRY_SHADER_EXT );

	//	// TODO: store and pass parameters

	//	////Setup Geometry Shader////
	//	// one of: GL_POINTS, GL_LINES, GL_LINES_ADJACENCY_EXT, GL_TRIANGLES, GL_TRIANGLES_ADJACENCY_EXT
	//	//Set GL_TRIANGLES primitives as INPUT
	//	//glProgramParameteriEXT( _programId,GL_GEOMETRY_INPUT_TYPE_EXT , GL_TRIANGLES );

	//	// one of: GL_POINTS, GL_LINE_STRIP, GL_TRIANGLE_STRIP  
	//	//Set TRIANGLE STRIP as OUTPUT
	//	//glProgramParameteriEXT( _programId,GL_GEOMETRY_OUTPUT_TYPE_EXT , GL_TRIANGLE_STRIP );

	//	// TODO: 
	//	// This parameter is very important and have a great impact on shader performance
	//	// Its value must be chosen closer as possible to real maximum number of vertices
	//	//glProgramParameteriEXT( _programId,GL_GEOMETRY_VERTICES_OUT_EXT, outputVertexCount );
	//	//std::cout << "GS output set to: " << outputVertexCount << "\n";
	//}

	if( !shaderOk )
		return false;

	// Link whole program object
	glLinkProgram( _programId );

	// Test link success
	GLint programOk = false;
	glGetProgramiv( _programId, GL_LINK_STATUS, &programOk );
	if( !programOk )
	{
		int32 maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetProgramInfoLog(_programId, maxLength, &maxLength, infoLog);
		std::cout << "Link error: " << infoLog << std::endl;
		delete []infoLog;
		return false;
	}

	// Program validation
	glValidateProgram( _programId );
	programOk = false;
	glGetProgramiv(_programId, GL_VALIDATE_STATUS, &programOk);
	if( !programOk )
	{
		int32 maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetProgramInfoLog(_programId, maxLength, &maxLength, infoLog);
		std::cout << "Validation error: " << infoLog << std::endl;
		delete []infoLog;
		return false;
	}

	// Cleanup
	glUseProgram( 0 );
	std::cout << std::endl << "***** Shaders loaded successfully *****" << std::endl << std::endl;
	return true;
}

void ShaderProgram::bind()
{
	glUseProgram( _programId );
}

void ShaderProgram::setUniform( const vr::String& symbolName, GLint value )
{
	GLint location = glGetUniformLocation( _programId, symbolName.data() );
	if( location == -1 )
	{
		std::cout << "Uniform \'" << symbolName << "\' not found!" << std::endl;
		return;
	}
	glUniform1i( location, value );
}

void ShaderProgram::setUniform( const vr::String& symbolName, GLfloat value )
{
	GLint location = glGetUniformLocation( _programId, symbolName.data() );
	if( location == -1 )
	{
		std::cout << "Uniform \'" << symbolName << "\' not found!" << std::endl;
		return;
	}
	glUniform1f( location, value );
}

void ShaderProgram::setUniform( const vr::String& symbolName, const GLfloat value[3], GLsizei count )
{
	GLint location = glGetUniformLocation( _programId, symbolName.data() );
	if( location == -1 )
	{
		std::cout << "Uniform \'" << symbolName << "\' not found!" << std::endl;
		return;
	}
	glUniform3fv( location, count, value );
}

void ShaderProgram::setUniform( const vr::String& symbolName, const GLint* value, GLsizei count )
{
	GLint location = glGetUniformLocation( _programId, symbolName.data() );
	if( location == -1 )
	{
		std::cout << "Uniform \'" << symbolName << "\' not found!" << std::endl;
		return;
	}
	glUniform1iv( location, count, value );
}

void ShaderProgram::release()
{
	glUseProgram( 0 );
}

/************************************************************************/
/* Private                                                              */
/************************************************************************/
bool ShaderProgram::loadShaderFile( GLhandleARB programObject, const vr::String& filename, GLuint type )
{
	// Source file reading
	std::string buff;
	std::ifstream file;
	std::cerr.flush();
	file.open( filename.data() );
	std::string line;
	while( std::getline(file, line) )
		buff += line + "\n";

	const GLcharARB *txt=buff.c_str();

	// Shader object creation
	GLhandleARB shader = glCreateShader(type);

	// Source code assignment
	glShaderSource(shader, 1, &txt, NULL);

	// Compile shader object
	glCompileShader(shader);

	// Check if shader compiled
	GLint ok = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		int32 maxLength=4096;
		char* infoLog = new char[maxLength];
		glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog);
		std::cout << "Compilation error: " << infoLog << std::endl;
		delete []infoLog;
		return false;
	}

	// attach shader to program object
	glAttachShader(programObject, shader);

	// delete object, no longer needed
	glDeleteShader(shader);

	// Global error checking
	const GLubyte* err = gluErrorString(glGetError());
	if( err == NULL )
		std::cout << "InitShader: Unknown error detected!" << std::endl;
	else
		std::cout << "InitShader: \'" << filename << "\' Errors: " << err << std::endl;
	return true;
}

void ShaderProgram::destroyShaders()
{
	GLint shaderCount;
	glGetProgramiv( _programId, GL_ATTACHED_SHADERS, &shaderCount );

	if( shaderCount > 0 )
	{
		std::vector<unsigned int> shaders( shaderCount );
		glGetAttachedShaders( _programId, shaderCount, NULL, &shaders[0] );
		for( int32 i = 0; i < shaderCount; ++i )
		{
			glDetachShader( _programId, shaders[i] );
			glDeleteShader( shaders[i] );
		}
	}
}

void ShaderProgram::destroy()
{
	if( _programId > 0 )
	{
		destroyShaders();
		glDeleteProgram( _programId );
	}
}
