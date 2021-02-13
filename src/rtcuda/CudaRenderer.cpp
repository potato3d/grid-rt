#include <rtc/CudaRenderer.h>
#include <rt/Context.h>
#include <rtp/PinholeCamera.h>
#include <rtp/UniformGridAccStruct.h>
#include "RayTracing.h"

using namespace rtc;

void CudaRenderer::newFrame()
{
	rtp::PinholeCamera* cam = dynamic_cast<rtp::PinholeCamera*>( rt::Context::current()->getCamera() );

	// Ray.orig = same for all rays (pinhole camera only)
	// Send camera parameters so that CUDA compute ray directions in parallel
	const vr::vec3f& position = cam->getPosition();
	const vr::vec3f& baseDir = cam->getBaseDir();
	const vr::vec3f& nearU = cam->getNearU();
	const vr::vec3f& nearV = cam->getNearV();
	cudaSetCameraParameters( position.x, position.y, position.z,
							 baseDir.x, baseDir.y, baseDir.z,
							 nearU.x, nearU.y, nearU.z,
							 nearV.x, nearV.y, nearV.z );

	// Get current viewport
	uint32 w;
	uint32 h;
	cam->getViewport( w, h );

	// Check if need to re-register frame buffer in CUDA
	updateCudaFrameBuffer( w, h );
}

void CudaRenderer::render()
{
	// Bind frame buffer texture and PBO
	glActiveTexture( GL_TEXTURE0 );
	_frameBuffer->bind();

	// Call CUDA ray tracing kernel
	cudaRayTrace();

	// Transfer updated PBO to GPU
	_frameBuffer->endWrite();

	// Do actual display of current frame
	glColor3f( 0, 1, 0 );
	glBegin( GL_QUADS );
		glTexCoord2f( 0, 0 );
		glVertex2f( 0, 0 );

		glTexCoord2f( 1, 0 );
		glVertex2f( 1, 0 );

		glTexCoord2f( 1, 1 );
		glVertex2f( 1, 1 );

		glTexCoord2f( 0, 1 );
		glVertex2f( 0, 1 );
	glEnd();

	// Deactivate texture and PBO
	_frameBuffer->release();
}

void CudaRenderer::initialize()
{
	rtgl::Texture* tex = rtgl::Texture::create();
	tex->init( rtgl::Texture::Format_RGBA_Ubyte, rtgl::Texture::Format_RGBA_Ubyte );

	_frameBuffer = rtgl::PixelBufferObject::create();
	_frameBuffer->init();
	_frameBuffer->setTexture( tex );

	// Register new frame buffer to CUDA
	updateCudaFrameBuffer( 512, 512 );

	// Transfer scene data to the GPU
	// TODO: support several geometries

	// Get first geometry from scene and send data to CUDA
	rt::Context* ctx = rt::Context::current();

	// Store triangle data
	const rt::Geometry& geometry = *ctx->getGeometry( 0 );

	//////////////////////////////////////////////////////////////////////////
	// Buffer 1: expanded triangle vertices
	//////////////////////////////////////////////////////////////////////////
	// Three vertices per triangle and four components per vertex (CUDA only works with 4-component textures)
	std::vector<float> bufferFloat( geometry.triDesc.size() * 3 * 4 );

	// Expand vertices according to triangle descriptions
	for( uint32 src = 0, dst = 0; src < geometry.triDesc.size(); ++src, dst+=12 )
	{
		// Vertex 0
		const vr::vec3f& v0 = geometry.getVertex( src, 0 );
		bufferFloat[dst]   = v0.x;
		bufferFloat[dst+1] = v0.y;
		bufferFloat[dst+2] = v0.z;
		bufferFloat[dst+3] = 1.0f; // dummy

		// Vertex 1
		const vr::vec3f& v1 = geometry.getVertex( src, 1 );
		bufferFloat[dst+4] = v1.x;
		bufferFloat[dst+5] = v1.y;
		bufferFloat[dst+6] = v1.z;
		bufferFloat[dst+7] = 1.0f; // dummy

		// Vertex 2
		const vr::vec3f& v2 = geometry.getVertex( src, 2 );
		bufferFloat[dst+8]  = v2.x;
		bufferFloat[dst+9]  = v2.y;
		bufferFloat[dst+10] = v2.z;
		bufferFloat[dst+11] = 1.0f; // dummy
	}

	cudaTransferTriangleVertices( &bufferFloat[0], bufferFloat.size() );

	//////////////////////////////////////////////////////////////////////////
	// Buffer 2: expanded triangle normals (same size as triangle vertices texture)
	//////////////////////////////////////////////////////////////////////////
	// Expand normals according to triangle descriptions
	for( uint32 src = 0, dst = 0; src < geometry.triDesc.size(); ++src, dst+=12 )
	{
		// Normal 0
		const vr::vec3f& n0 = geometry.getNormal( src, 0 );
		bufferFloat[dst]   = n0.x;
		bufferFloat[dst+1] = n0.y;
		bufferFloat[dst+2] = n0.z;
		bufferFloat[dst+3] = 1.0f; // dummy

		// Normal 1
		const vr::vec3f& n1 = geometry.getNormal( src, 1 );
		bufferFloat[dst+4] = n1.x;
		bufferFloat[dst+5] = n1.y;
		bufferFloat[dst+6] = n1.z;
		bufferFloat[dst+7] = 1.0f; // dummy

		// Normal 2
		const vr::vec3f& n2 = geometry.getNormal( src, 2 );
		bufferFloat[dst+8]  = n2.x;
		bufferFloat[dst+9]  = n2.y;
		bufferFloat[dst+10] = n2.z;
		bufferFloat[dst+11] = 1.0f; // dummy
	}

	cudaTransferTriangleNormals( &bufferFloat[0], bufferFloat.size() );

	// Don't need this buffer anymore
	vr::vectorFreeMemory( bufferFloat );

	// Store acceleration structure
	const rtp::UniformGridAccStruct* grid = dynamic_cast<const rtp::UniformGridAccStruct*>( geometry.accStruct.get() );

	int32 nx;
	int32 ny;
	int32 nz;
	grid->getResolution( nx, ny, nz );

	//////////////////////////////////////////////////////////////////////////
	// Buffer 3: grid cell pointers (point to Buffer 4)
	//////////////////////////////////////////////////////////////////////////
	std::vector<int32> cellPointers( nx * ny * nz * 2 );

	//////////////////////////////////////////////////////////////////////////
	// Buffer 4: grid cell triangle ids (point to Buffer 1)
	//////////////////////////////////////////////////////////////////////////
	std::vector<int32> cellTriangleIds;

	uint32 currentPos = 0;

	for( int32 z = 0; z < nz; ++z )
	{
		for( int32 y = 0; y < ny; ++y )
		{
			for( int32 x = 0; x < nx; ++x )
			{
				const rtp::UniformGridAccStruct::Cell& cell = grid->at( x, y, z );

				// Store start position of triangle ids
				cellPointers[currentPos] = cellTriangleIds.size();

				// Set element count
				cellPointers[currentPos+1] = cell.size();

				// Append this cell's triangle ids to texture
				for( uint32 i = 0, limit = cell.size(); i < limit; ++i )
				{
					// Since we have expanded the vertices and normals, triangle i is at position i*3
					cellTriangleIds.push_back( cell[i] * 3 );
				}

				currentPos+=2;
			}
		}
	}

	cudaTransferCellPointers( &cellPointers[0], cellPointers.size() );
	cudaTransferCellTriangleIds( &cellTriangleIds[0], cellTriangleIds.size() );

	// Remaining grid description
	const vr::vec3f& minv = grid->getBoundingBox().minv;
	const vr::vec3f& maxv = grid->getBoundingBox().maxv;
	cudaSetBoxMin( minv.x, minv.y, minv.z );
	cudaSetBoxMax( maxv.x, maxv.y, maxv.z );

	const vr::vec3f& cellSize = grid->getCellSize();
	cudaSetGridParameters( cellSize.x, cellSize.y, cellSize.z, nx, ny, nz );
}

// Private
void CudaRenderer::updateCudaFrameBuffer( uint32 width, uint32 height )
{
	if( width  == _frameBuffer->getTexture()->getWidth() &&
		height == _frameBuffer->getTexture()->getHeight() )
		return;

	cudaUnregisterBuffer( _frameBuffer->getBufferId() );

	_frameBuffer->reallocate( GL_STREAM_COPY, width, height );

	cudaRegisterBuffer( _frameBuffer->getBufferId() );

	cudaSetScreenSize( _frameBuffer->getTexture()->getWidth(), _frameBuffer->getTexture()->getHeight() );
}
