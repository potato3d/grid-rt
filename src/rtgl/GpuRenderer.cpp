#include <rtgl/GpuRenderer.h>
#include <rt/Context.h>
#include <rt/Scene.h>
#include <rt/Geometry.h>
#include <rt/ICamera.h>

#include <rtp/UniformGridAccStruct.h>
#include <rtp/PhongMaterialColor.h>
#include <rtp/SimplePointLight.h>
#include <rtp/Texture2D.h>

#include <util.h>

#include <map>

#include <cassert>

using namespace rtgl;

GLuint texId1;
GLuint texId2;
GLuint texId3;

#define ENABLE_REFLECTION 0
//#define USE_PBO

int rotateRight( int x, int start, int end )
{
	++x;
	return x > end ? start : x;
}

GpuRenderer::GpuRenderer()
{
	_animationEnabled = true;
}

void GpuRenderer::setAnimationEnabled( bool enabled )
{
	_animationEnabled = enabled;
}

void GpuRenderer::newFrame()
{
	rt::ICamera* cam = rt::Context::current()->getCamera();
	uint32 w;
	uint32 h;
	cam->getViewport( w, h );

	// Ray.orig = uniform (pinhole camera only)
	vr::vec3f rayOrigin;
	cam->getRayOrigin( rayOrigin, 0, 0 );

	_shaders.bind();
	_shaders.setUniform( "u_rayOrig", rayOrigin.ptr );
	_shaders.release();

	_shaders2.bind();
	_shaders2.setUniform( "u_rayOrig", rayOrigin.ptr );
	_shaders2.release();

	_shaders3.bind();
	_shaders3.setUniform( "u_rayOrig", rayOrigin.ptr );
	_shaders3.release();

	// Ray.dir = send as texcoord the ray directions of the 4 corners of camera screen quad
	// Rasterizer interpolates them automatically
	cam->getRayDirection( _lowerLeftRayDir,  0.0f, 0.0f );
	cam->getRayDirection( _lowerRightRayDir,    w, 0.0f );
	cam->getRayDirection( _upperRightRayDir,    w,    h );
	cam->getRayDirection( _upperLeftRayDir,  0.0f,    h );
}

void GpuRenderer::render()
{
	int numKeyFrames = objAnimation.frameObjs.size();

	// if no animation loaded, skip
	if( numKeyFrames == 0 )
		return;

	//fprintf( getLogFile(), "----- Begin Frame -----\n" );

//printf( "----- Begin Frame -----\n" );

	vr::Timer t;

	glFinish();

	// Bind triangle vertices
	glActiveTexture( GL_TEXTURE0 );
	_triangleVertices[_currBufferId]->bind();

	// Update current frame
	const double FRAME_TIME = 0.1;
	double elapsed = _frameTimer.elapsed();
	if( _animationEnabled && elapsed > FRAME_TIME )
	{
		int newFrameId = rotateRight( _currFrameId, 0, numKeyFrames - 1 );

		if( newFrameId != _currFrameId )
		{
			_currFrameId = newFrameId;

			_frameTimer.restart();

			//fprintf( getLogFile(), "upload new keyframe:  %d\n", _currFrameId );

			t.restart();

			uploadKeyframe( _currFrameId, _currBufferId );

			glFinish();

			printf( "upload time: %.3f ms\n", t.elapsed() * 1e3 );

			//fprintf( getLogFile(), "begin grid rebuild...\n" );

			t.restart();

			// Only update grid if current key frame changed
			_grid.rebuildGrid();

			glFinish();

			printf( "rebuild time: %.3f ms\n", t.elapsed() * 1e3 );

			//fprintf( getLogFile(), "grid rebuild done\n" );
		}
	}

	//fprintf( getLogFile(), "begin render... " );

	t.restart();

	rt::ICamera* cam = rt::Context::current()->getCamera();
	uint32 w;
	uint32 h;
	cam->getViewport( w, h );
	glViewport( 0, 0, w, h );

	// Bind textures in each unit
	
	glActiveTexture( GL_TEXTURE1 );
	_triangleNormals[_currBufferId]->bind();
	glActiveTexture( GL_TEXTURE2 );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, _grid._texGridAccum );
	glActiveTexture( GL_TEXTURE3 );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, _grid._texGridData );
	glActiveTexture( GL_TEXTURE4 );
	_materials[_currBufferId]->bind();
	glActiveTexture( GL_TEXTURE5 );
	_triangleTexCoords[_currBufferId]->bind();

	// Bind material textures, if any
	glActiveTexture( GL_TEXTURE6 );
	glBindTexture( GL_TEXTURE_2D_ARRAY_EXT, _materialTextureArray );

	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _auxFBO );
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, texId1, 0 );

	// Bind shaders
	_shaders.bind();
	_shaders.setUniform( "u_reflectionPass", false );

	// Draw quad to trigger fragment processors
	glBegin( GL_QUADS );
		glMultiTexCoord3fv( GL_TEXTURE0, _lowerLeftRayDir.ptr );
		glVertex2f( 0, 0 );

		glMultiTexCoord3fv( GL_TEXTURE0, _lowerRightRayDir.ptr );
		glVertex2f( 1, 0 );

		glMultiTexCoord3fv( GL_TEXTURE0, _upperRightRayDir.ptr );
		glVertex2f( 1, 1 );

		glMultiTexCoord3fv( GL_TEXTURE0, _upperLeftRayDir.ptr );
		glVertex2f( 0, 1 );
	glEnd();

	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, texId2, 0 );

	_shaders2.bind();
	_shaders2.setUniform( "u_reflectionPass", false );

	// Draw quad to trigger fragment processors
	glBegin( GL_QUADS );
		glMultiTexCoord3fv( GL_TEXTURE0, _lowerLeftRayDir.ptr );
		glVertex2f( 0, 0 );

		glMultiTexCoord3fv( GL_TEXTURE0, _lowerRightRayDir.ptr );
		glVertex2f( 1, 0 );

		glMultiTexCoord3fv( GL_TEXTURE0, _upperRightRayDir.ptr );
		glVertex2f( 1, 1 );

		glMultiTexCoord3fv( GL_TEXTURE0, _upperLeftRayDir.ptr );
		glVertex2f( 0, 1 );
	glEnd();

	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

	_shaders3.bind();
	_shaders3.setUniform( "u_reflectionPass", false );

	// Draw quad to trigger fragment processors
	glBegin( GL_QUADS );
		glMultiTexCoord3fv( GL_TEXTURE0, _lowerLeftRayDir.ptr );
		glVertex2f( 0, 0 );

		glMultiTexCoord3fv( GL_TEXTURE0, _lowerRightRayDir.ptr );
		glVertex2f( 1, 0 );

		glMultiTexCoord3fv( GL_TEXTURE0, _upperRightRayDir.ptr );
		glVertex2f( 1, 1 );

		glMultiTexCoord3fv( GL_TEXTURE0, _upperLeftRayDir.ptr );
		glVertex2f( 0, 1 );
	glEnd();

	_shaders3.release();











#if ENABLE_REFLECTION

	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _auxFBO );
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, texId3, 0 );

	// Bind shaders
	_shaders.bind();
	_shaders.setUniform( "u_reflectionPass", true );

	// Draw quad to trigger fragment processors
	glBegin( GL_QUADS );
	glMultiTexCoord3fv( GL_TEXTURE0, _lowerLeftRayDir.ptr );
	glVertex2f( 0, 0 );

	glMultiTexCoord3fv( GL_TEXTURE0, _lowerRightRayDir.ptr );
	glVertex2f( 1, 0 );

	glMultiTexCoord3fv( GL_TEXTURE0, _upperRightRayDir.ptr );
	glVertex2f( 1, 1 );

	glMultiTexCoord3fv( GL_TEXTURE0, _upperLeftRayDir.ptr );
	glVertex2f( 0, 1 );
	glEnd();

	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, texId2, 0 );

	_shaders2.bind();
	_shaders2.setUniform( "u_reflectionPass", true );

	// Draw quad to trigger fragment processors
	glBegin( GL_QUADS );
	glMultiTexCoord3fv( GL_TEXTURE0, _lowerLeftRayDir.ptr );
	glVertex2f( 0, 0 );

	glMultiTexCoord3fv( GL_TEXTURE0, _lowerRightRayDir.ptr );
	glVertex2f( 1, 0 );

	glMultiTexCoord3fv( GL_TEXTURE0, _upperRightRayDir.ptr );
	glVertex2f( 1, 1 );

	glMultiTexCoord3fv( GL_TEXTURE0, _upperLeftRayDir.ptr );
	glVertex2f( 0, 1 );
	glEnd();

	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

	glEnable( GL_BLEND );
	glBlendFunc( GL_ONE, GL_ONE );

	_shaders3.bind();
	_shaders3.setUniform( "u_reflectionPass", true );

	// Draw quad to trigger fragment processors
	glBegin( GL_QUADS );
	glMultiTexCoord3fv( GL_TEXTURE0, _lowerLeftRayDir.ptr );
	glVertex2f( 0, 0 );

	glMultiTexCoord3fv( GL_TEXTURE0, _lowerRightRayDir.ptr );
	glVertex2f( 1, 0 );

	glMultiTexCoord3fv( GL_TEXTURE0, _upperRightRayDir.ptr );
	glVertex2f( 1, 1 );

	glMultiTexCoord3fv( GL_TEXTURE0, _upperLeftRayDir.ptr );
	glVertex2f( 0, 1 );
	glEnd();

	_shaders3.release();

	glDisable( GL_BLEND );

#endif





	//fprintf( getLogFile(), "done\n" );

	//fprintf( getLogFile(), "gl errors: %s\n", gluErrorString( glGetError() ) );

	//printf( "----- End Frame -----\n" );
	//fprintf( getLogFile(), "----- End Frame -----\n" );
	
	glFinish();
	printf( "render time: %.3f ms\n", t.elapsed() * 1e3 );
}

void GpuRenderer::update()
{
	int numKeyFrames = objAnimation.frameObjs.size();

	// If no animation loaded, skip
	if( numKeyFrames == 0 )
		return;

	_currFrameId = numKeyFrames - 1;
	_currBufferId = 0;

	//fprintf( getLogFile(), "resize CPU data\n" );

	//glClampColorARB( GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE );
	//glClampColorARB( GL_CLAMP_READ_COLOR_ARB, GL_FALSE );

#ifdef USE_PBO
	glClampColorARB( GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE );
#endif

	// Setup GLSL data for all keyframes
	for( int i = 0; i < numKeyFrames; ++i )
	{
		KeyFrameObj* obj = objAnimation.frameObjs[i].get();

		assert( obj->vertices.size() == obj->normals.size() );
		assert( obj->vertices.size() == obj->texcoords.size() );
		assert( obj->vertices.size() % 3 == 0 );
		assert( obj->materials.size() == obj->vertices.size() / 3 );

		int texW, texH;
		convertTo2dSize( obj->vertices.size(), MAX_TEX_WIDTH, texW, texH );
		vr::vectorExactResize( obj->vertices, texW * texH, vr::vec3f::ZERO() );

		vr::vectorExactResize( obj->normals, texW * texH, vr::vec3f::ZERO() );

		vr::vectorExactResize( obj->texcoords, texW * texH );

		convertTo2dSize( obj->materials.size(), MAX_TEX_WIDTH, texW, texH );
		vr::vectorExactResize( obj->materials, texW * texH );
	}

	//fprintf( getLogFile(), "allocate GPU textures\n" );

	//for( int i = 0; i < 2; ++i )
	{
		KeyFrameObj* obj = objAnimation.frameObjs[0].get();

		int texW, texH;
		convertTo2dSize( obj->vertices.size(), MAX_TEX_WIDTH, texW, texH );

		_triangleVertices.push_back( Texture::create() );
		_triangleNormals.push_back( Texture::create() );
		_triangleTexCoords.push_back( Texture::create() );
		_materials.push_back( Texture::create() );

		_triangleVertices.back()->init( Texture::Format_RGB_Float, Texture::Format_RGB_Float, Texture::Target_2D_Unormalized );
		_triangleVertices.back()->setImage( (float*)&obj->vertices[0], texW, texH );

#ifdef USE_PBO
		GLuint id;
		glGenBuffers( 1, &id );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, id );
		glBufferData( GL_PIXEL_UNPACK_BUFFER, texW * texH * 3 * sizeof(float), NULL, GL_STREAM_DRAW );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
		_triangleVerticesPBO.push_back( id );
#endif
		
		_triangleNormals.back()->init( Texture::Format_RGB_Float, Texture::Format_RGB_Float, Texture::Target_2D_Unormalized );
		_triangleNormals.back()->setImage( (float*)&obj->normals[0], texW, texH );

#ifdef USE_PBO
		glGenBuffers( 1, &id );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, id );
		glBufferData( GL_PIXEL_UNPACK_BUFFER, texW * texH * 3 * sizeof(float), NULL, GL_STREAM_DRAW );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
		_triangleNormalsPBO.push_back( id );
#endif

		_triangleTexCoords.back()->init( Texture::Format_Luminance_Alpha_Float, Texture::Format_Luminance_Alpha_Float, Texture::Target_2D_Unormalized );
		_triangleTexCoords.back()->setImage( (float*)&obj->texcoords[0], texW, texH );

#ifdef USE_PBO
		glGenBuffers( 1, &id );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, id );
		glBufferData( GL_PIXEL_UNPACK_BUFFER, texW * texH * 2 * sizeof(float), NULL, GL_STREAM_DRAW );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
		_triangleTexCoordsPBO.push_back( id );
#endif

		convertTo2dSize( obj->materials.size(), MAX_TEX_WIDTH, texW, texH );

		_materials.back()->init( Texture::Format_RGBA_Float, Texture::Format_RGBA_Float, Texture::Target_2D_Unormalized );
		_materials.back()->setImage( (float*)&obj->materials[0], texW, texH );

#ifdef USE_PBO
		glGenBuffers( 1, &id );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, id );
		glBufferData( GL_PIXEL_UNPACK_BUFFER, texW * texH * 4 * sizeof(float), NULL, GL_STREAM_DRAW );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
		_materialsPBO.push_back( id );
#endif
	}

	//fprintf( getLogFile(), "resize material textures\n" );

	//////////////////////////////////////////////////////////////////////////
	// Send regular material textures to GPU
	//////////////////////////////////////////////////////////////////////////
	int numTextures = objAnimation.textures.size();

	if( numTextures > 0 )
	{
		std::vector<unsigned char> texels;
		//int maxW = -INT_MAX;
		//int maxH = -INT_MAX;
		int maxW = 0;
		int maxH = 0;

		// All textures inside array must be of equal size,
		// so get maximum dimension to be used in texture array
		// TODO: using average dimension to save memory
		for( int i = 0; i < numTextures; ++i )
		{
			const Tex2D& tex = objAnimation.textures[i];
			//maxW = vr::max( maxW, tex.w );
			//maxH = vr::max( maxH, tex.h );
			maxW += tex.w;
			maxH += tex.h;
		}

		// TODO: 
		maxW = 512;//(float)maxW / (float)numTextures;
		maxH = 512;//(float)maxH / (float)numTextures;

		// Scale all images to be of same size
		for( int i = 0; i < numTextures; ++i )
		{
			const Tex2D& tex = objAnimation.textures[i];

			unsigned int imgStartIdx = texels.size();
			texels.resize( texels.size() + maxW * maxH * 4 );

			gluScaleImage( GL_RGBA, 
				tex.w, tex.h, GL_UNSIGNED_BYTE, &tex.texels[0], 
				maxW, maxH, GL_UNSIGNED_BYTE, &texels[imgStartIdx] );
		}

		//fprintf( getLogFile(), "uploading textures to GPU (%.1f MB)... ", maxW * maxH * numTextures * 4 * sizeof(char) * 0.00000095367431640625 );
#if TRACK_ALLOC
		fgetc(stdin);
#endif

		// Create and store texture array
		glGenTextures( 1, &_materialTextureArray );
		glBindTexture( GL_TEXTURE_2D_ARRAY_EXT, _materialTextureArray );
		glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glTexImage3D( GL_TEXTURE_2D_ARRAY_EXT, 0, GL_RGBA, maxW, maxH, numTextures, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texels[0] );
		glBindTexture( GL_TEXTURE_2D_ARRAY_EXT, 0 );

		//fprintf( getLogFile(), "done\n" );
	}

	//////////////////////////////////////////////////////////////////////////
	// Store acceleration structure
	//////////////////////////////////////////////////////////////////////////
	//fprintf( getLogFile(), "\nbuilding GPU grid...\n" );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

// TODO: fixes loading other .obj files, who knows why...
glGetError();
//printf( "-> %s\n", gluErrorString( glGetError() ) );

	rebuildGrid( true );

	// Setup shaders
	_shaders.create();
	_shaders.setVertexFile(   "../shaders/uniformGridRayTracer_VERT.glsl" );
	//_shaders.setFragmentFile( "../shaders/uniformGridRayTracer_FRAG.glsl" );
	_shaders.setFragmentFile( "../shaders/traversalIntersection_FRAG.glsl" );

	_shaders2.create();
	_shaders2.setVertexFile(   "../shaders/uniformGridRayTracer_VERT.glsl" );
	_shaders2.setFragmentFile( "../shaders/shadow_FRAG.glsl" );

	_shaders3.create();
	_shaders3.setVertexFile(   "../shaders/uniformGridRayTracer_VERT.glsl" );
	_shaders3.setFragmentFile( "../shaders/shading_FRAG.glsl" );


	reloadShaders();


/*
	//////////////////////////////////////////////////////////////////////////
	// Texture 3: material information
	//////////////////////////////////////////////////////////////////////////
	_materials->init( rtgl::Texture::Format_RGBA_Float, rtgl::Texture::Format_RGBA_Float, 
		              rtgl::Texture::Target_2D_Unormalized );

	// Allocate space for a 4-component texture with 1 texel per triangle
	convertTo2dSize( numTriangles, MAX_TEX_WIDTH, numTexColumns, numTexLines );
	vr::vectorExactResize( texelBuffer, numTexColumns * numTexLines * 4, 0.0f );

	// For each triangle, store diffuse material component
	for( uint32 src = 0, dst = 0; src < numTriangles; ++src, dst+=4 )
	{
		rt::IMaterial* rawMat = geometry.triDesc[src].material;

		// Try to read phong material color
		rtp::PhongMaterialColor* mat = dynamic_cast<rtp::PhongMaterialColor*>( rawMat );
		if( mat != NULL )
		{
			texelBuffer[dst+0] = mat->_diffuse.x;
			texelBuffer[dst+1] = mat->_diffuse.y;
			texelBuffer[dst+2] = mat->_diffuse.z;
			texelBuffer[dst+3] = (float)mat->_textureId - 1.0f;
		}
		else
		{
			texelBuffer[dst+0] = 0.0f;
			texelBuffer[dst+1] = 0.0f;
			texelBuffer[dst+2] = 1.0f;
			texelBuffer[dst+3] = -1.0f;
		}
	}

	_materials->setImage( &texelBuffer[0], numTexColumns, numTexLines );
*/



	//////////////////////////////////////////////////////////////////////////
	// TODO: TESTE
	rt::ICamera* cam = rt::Context::current()->getCamera();
	uint32 w;
	uint32 h;
	cam->getViewport( w, h );

	glGenTextures( 1, &texId1 );
	glActiveTexture( GL_TEXTURE7 );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, texId1 );
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, w, h, 0, GL_RGBA, GL_FLOAT, NULL );

	glGenTextures( 1, &texId2 );
	glActiveTexture( GL_TEXTURE8 );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, texId2 );
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, w, h, 0, GL_RGBA, GL_FLOAT, NULL );

	glGenTextures( 1, &texId3 );
	glActiveTexture( GL_TEXTURE9 );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, texId3 );
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, w, h, 0, GL_RGBA, GL_FLOAT, NULL );

	glGenFramebuffersEXT( 1, &_auxFBO );
}

void GpuRenderer::reloadShaders()
{
	_shaders.reloadShaders();

	// Set texture units and time-invariant data
	_shaders.bind();

	_shaders.setUniform( "u_texTriangleVertices",  0 );
	_shaders.setUniform( "u_texTriangleNormals",   1 );
	_shaders.setUniform( "u_texCellPointers",      2 );
	_shaders.setUniform( "u_texCellTriangleIds",   3 );

	_shaders.setUniform( "u_boxMin", objAnimation.bbox.minv.ptr );
	_shaders.setUniform( "u_boxMax", objAnimation.bbox.maxv.ptr );

	vr::vec3f invCellSize( 1.0f / _grid._cellSize.x, 1.0f / _grid._cellSize.y, 1.0f / _grid._cellSize.z );
	vr::vec3f gridSize( _grid._numCells.x, _grid._numCells.y, _grid._numCells.z );

	_shaders.setUniform( "u_cellSize", _grid._cellSize.ptr );
	_shaders.setUniform( "u_invCellSize", invCellSize.ptr );
	_shaders.setUniform( "u_gridSize", gridSize.ptr );

	_shaders.setUniform( "u_reflectionPass", false );
	_shaders.setUniform( "u_texHits", 8 );

	_shaders.release();



	_shaders2.reloadShaders();

	// Set texture units and time-invariant data
	_shaders2.bind();

	_shaders2.setUniform( "u_texTriangleVertices",  0 );
	_shaders2.setUniform( "u_texTriangleNormals",   1 );
	_shaders2.setUniform( "u_texCellPointers",      2 );
	_shaders2.setUniform( "u_texCellTriangleIds",   3 );
	_shaders2.setUniform( "u_texHits", 7 );
	_shaders2.setUniform( "u_texHitsReflect", 9 );

	_shaders2.setUniform( "u_boxMin", objAnimation.bbox.minv.ptr );

	_shaders2.setUniform( "u_cellSize", _grid._cellSize.ptr );
	_shaders2.setUniform( "u_invCellSize", invCellSize.ptr );
	_shaders2.setUniform( "u_gridSize", gridSize.ptr );

	_shaders2.release();



	_shaders3.reloadShaders();

	// Set texture units and time-invariant data
	_shaders3.bind();

	_shaders3.setUniform( "u_texTriangleNormals",   1 );
	_shaders3.setUniform( "u_texMaterials",         4 );
	_shaders3.setUniform( "u_texTriangleTexcoords", 5 );
	_shaders3.setUniform( "u_texMaterialTextureArray", 6 );
	_shaders3.setUniform( "u_texHits", 8 );

	_shaders3.release();
}

void GpuRenderer::rebuildGrid( bool recomputeResolution )
{
	_grid.clear();
	_grid.init( objAnimation.frameObjs[0]->numTriangles, objAnimation.bbox.minv, objAnimation.bbox.maxv, false, recomputeResolution );

	vr::Timer t;

	///glFinish();

	t.restart();

	glActiveTexture( GL_TEXTURE0 );
	_triangleVertices[_currBufferId]->bind();

	_grid.rebuildGrid();

	///printf( "rebuild time: %.3f ms\n", t.elapsed() * 1e3 );

	//fprintf( getLogFile(), "\n***** GPU uniform Grid *****\n" );
	//fprintf( getLogFile(), "numCells: %d, %d, %d (%d cells)\n", _grid._numCells.x, _grid._numCells.y, _grid._numCells.z, _grid._numCells.x * _grid._numCells.y * _grid._numCells.z );

#if 0
	// TODO: check grid
	checkGpuGrid( _grid, ((rtp::UniformGridAccStruct*)geometry.accStruct.get())->getGridData() );
	exit( 1 );
#endif

	//CpuGrid cpuGrid;
	//cpuGrid.setBoxMin( objAnimation.bbox.minv.x, objAnimation.bbox.minv.y, objAnimation.bbox.minv.z );
	//cpuGrid.setBoxMax( objAnimation.bbox.maxv.x, objAnimation.bbox.maxv.y, objAnimation.bbox.maxv.z );
	//cpuGrid.setNumTriangles( objAnimation.frameObjs[0]->vertices.size() / 3, false );

	//for( int i = 0; i < 5; ++i )
	//{
	//	t.restart();

	//	cpuGrid.rebuild( (float*)&objAnimation.frameObjs[0]->vertices[0], objAnimation.frameObjs[0]->vertices.size() / 3 );

	//	printf( "cpu rebuild time: %.3f ms\n", t.elapsed() * 1e3 );
	//}
}

void GpuRenderer::uploadKeyframe( int frameId, int texId )
{
	KeyFrameObj* obj = objAnimation.frameObjs[frameId].get();

	////////////////////////////////////////////////////////////////////////////
	// Texture 1: expanded triangle vertices
	////////////////////////////////////////////////////////////////////////////

	int texW, texH;
	convertTo2dSize( obj->vertices.size(), MAX_TEX_WIDTH, texW, texH );

	glActiveTexture( GL_TEXTURE14 );

	//fprintf( getLogFile(), "uploading vertices to GPU (%.1f MB)... ", texW * texH * 3 * sizeof(float) * 0.00000095367431640625 );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

#ifdef USE_PBO
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _triangleVerticesPBO[texId] );
	glBufferSubData( GL_PIXEL_UNPACK_BUFFER, 0, texW * texH * 3 * sizeof(float), (float*)&obj->vertices[0] );
	_triangleVertices[texId]->setSubImage( NULL, texW, texH );
#else
	_triangleVertices[texId]->setSubImage( (float*)&obj->vertices[0], texW, texH );
#endif

	//fprintf( getLogFile(), "done\n" );

	////////////////////////////////////////////////////////////////////////////
	// Texture 2: expanded triangle normals (same size as triangle vertices texture)
	////////////////////////////////////////////////////////////////////////////

	//fprintf( getLogFile(), "uploading normals to GPU (%.1f MB)... ", texW * texH * 3 * sizeof(float) * 0.00000095367431640625 );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

#ifdef USE_PBO
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _triangleNormalsPBO[texId] );
	glBufferSubData( GL_PIXEL_UNPACK_BUFFER, 0, texW * texH * 3 * sizeof(float), (float*)&obj->normals[0] );
	_triangleNormals[texId]->setSubImage( NULL, texW, texH );
#else
	_triangleNormals[texId]->setSubImage( (float*)&obj->normals[0], texW, texH );
#endif

	//fprintf( getLogFile(), "done\n" );

	//////////////////////////////////////////////////////////////////////////
	// Texture 3: expanded texture coordinates (same size as triangle vertices texture)
	//////////////////////////////////////////////////////////////////////////

	//fprintf( getLogFile(), "uploading texcoords to GPU (%.1f MB)... ", texW * texH * 2 * sizeof(float) * 0.00000095367431640625 );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

#ifdef USE_PBO
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _triangleTexCoordsPBO[texId] );
	glBufferSubData( GL_PIXEL_UNPACK_BUFFER, 0, texW * texH * 2 * sizeof(float), (float*)&obj->texcoords[0] );
	_triangleTexCoords[texId]->setSubImage( NULL, texW, texH );
#else
	_triangleTexCoords[texId]->setSubImage( (float*)&obj->texcoords[0], texW, texH );
#endif

	//fprintf( getLogFile(), "done\n" );

	//////////////////////////////////////////////////////////////////////////
	// Texture 4: material description (size = num triangles)
	//////////////////////////////////////////////////////////////////////////

	convertTo2dSize( obj->materials.size(), MAX_TEX_WIDTH, texW, texH );

	//fprintf( getLogFile(), "uploading materials to GPU (%.1f MB)... ", texW * texH * 4 * sizeof(float) * 0.00000095367431640625 );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

#ifdef USE_PBO
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _materialsPBO[texId] );
	glBufferSubData( GL_PIXEL_UNPACK_BUFFER, 0, texW * texH * 4 * sizeof(float), (float*)&obj->materials[0] );
	_materials[texId]->setSubImage( NULL, texW, texH );
#else
	_materials[texId]->setSubImage( (float*)&obj->materials[0], texW, texH );
#endif

	//fprintf( getLogFile(), "done\n" );

#ifdef USE_PBO
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
#endif
}
