#ifndef _RTC_RAYTRACING_H_
#define _RTC_RAYTRACING_H_

#include <gl/glew.h>

extern "C" {

bool rtInitContext();

// Register PBO to CUDA
void cudaRegisterBuffer( GLuint bufferId );

// Unregister PBO back to OpenGL
void cudaUnregisterBuffer( GLuint bufferId );

// Set screen dimensions (size of PBO)
void cudaSetScreenSize( uint32 width, uint32 height );

// Set camera parameters so CUDA computes ray directions in parallel
void cudaSetCameraParameters( float posX, float posY, float posZ, 
							  float baseDirX, float baseDirY, float baseDirZ,
							  float uNearX, float uNearY, float uNearZ, 
							  float vNearX, float vNearY, float vNearZ );


// Scene data
void cudaTransferTriangleVertices( float* vertices, uint32 size );
void cudaTransferTriangleNormals( float* normals, uint32 size );

// Acceleration structure data
void cudaTransferCellPointers( int32* cellPointers, uint32 size );
void cudaTransferCellTriangleIds( int32* cellTriangleIds, uint32 size );
void cudaSetBoxMin( float x, float y, float z );
void cudaSetBoxMax( float x, float y, float z );
void cudaSetGridParameters( float cellSizeX, float cellSizeY, float cellSizeZ,
						    int32 numCellsX, int32 numCellsY, int32 numCellsZ );

// Set block size for computations
void cudaSetBlockSize( uint32 width, uint32 height );

// Call ray tracing kernel
void cudaRayTrace();

} // extern "C"

#endif // _RTC_RAYTRACING_H_
