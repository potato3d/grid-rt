// Relevant includes
#include <rtc/common.h>
#include <rtc/CudaDependencies.h>
#include "KernelDependencies.h"

// Include kernel
#include "RayTracing_kernel.cu"

// Forward declaration
#include "RayTracing.h"

#include <cudpp.h>

////////////////////////////////////////////////////////////////////////////////
// Globals
static GLuint g_frameBufferId = 0;

// Texture data
static int2*   g_texCellPointersData = NULL;
static int*    g_texCellTriangleIdsData = NULL;
static float4* g_texVerticesData = NULL;
static float4* g_texNormalsData = NULL;

// Camera
static Camera g_camera;

// Acceleration structure data
static Grid g_grid;

// TODO: 
//static int    g_vertexCount = 0;
//static float* g_triangleCounters = NULL;
//static float* g_triangleIndices = NULL;
//
//static CUDPPScanConfig g_scanConfig;


////////////////////////////////////////////////////////////////////////////////
// Entry point for Cuda functionality on host side
////////////////////////////////////////////////////////////////////////////////
extern "C" {

bool rtInitContext()
{
	int dummyArgc = 0;
	const char** dummyArgv = NULL;

	// Initialize CUDA context
	CUT_DEVICE_INIT( dummyArgc, dummyArgv );

	// Everything ok
	return true;
}

void cudaRegisterBuffer( GLuint bufferId )
{
	if( g_frameBufferId != 0 )
		cudaUnregisterBuffer( bufferId );

	CUDA_SAFE_CALL( cudaGLRegisterBufferObject( bufferId ) );
	CUT_CHECK_ERROR( "cudaGLRegisterBufferObject failed.\n" );
	g_frameBufferId = bufferId;
}

void cudaUnregisterBuffer( GLuint bufferId )
{
	if( bufferId != g_frameBufferId )
		return;

	CUDA_SAFE_CALL( cudaGLUnregisterBufferObject( bufferId ) );
	CUT_CHECK_ERROR( "cudaGLUnregisterBufferObject failed.\n" );
	g_frameBufferId = 0;
}

void cudaSetScreenSize( uint32 width, uint32 height )
{
	g_camera.screenWidth = width;
	g_camera.screenHeight = height;

	g_camera.invScreenWidth = 1.0f / (float)width;
	g_camera.invScreenHeight = 1.0f / (float)height;
}

void cudaSetCameraParameters( float posX, float posY, float posZ, 
							  float baseDirX, float baseDirY, float baseDirZ,
							  float uNearX, float uNearY, float uNearZ, 
							  float vNearX, float vNearY, float vNearZ )
{
	g_camera.position.x = posX;
	g_camera.position.y = posY;
	g_camera.position.z = posZ;

	g_camera.baseDir.x = baseDirX;
	g_camera.baseDir.y = baseDirY;
	g_camera.baseDir.z = baseDirZ;

	g_camera.nearU.x = uNearX;
	g_camera.nearU.y = uNearY;
	g_camera.nearU.z = uNearZ;

	g_camera.nearV.x = vNearX;
	g_camera.nearV.y = vNearY;
	g_camera.nearV.z = vNearZ;
}

void cudaTransferTriangleVertices( float* vertices, uint32 size )
{
	//g_vertexCount = size / 4;

	uint32 byteTotal = size*sizeof(float);

	// Malloc linear memory
	CUDA_SAFE_CALL( cudaMalloc( (void**)&g_texVerticesData, byteTotal ) );
	CUT_CHECK_ERROR( "cudaMalloc failed.\n" );

	// Memcpy to linear memory
	CUDA_SAFE_CALL( cudaMemcpy( g_texVerticesData, vertices, byteTotal, cudaMemcpyHostToDevice ) );
	CUT_CHECK_ERROR( "cudaMemcpy failed.\n" );

	// Bind the data to the texture
    CUDA_SAFE_CALL( cudaBindTexture( 0, texVertices, g_texVerticesData, byteTotal ) );
}

void cudaTransferTriangleNormals( float* normals, uint32 size )
{
	uint32 byteTotal = size*sizeof(float);

	// Malloc linear memory
	CUDA_SAFE_CALL( cudaMalloc( (void**)&g_texNormalsData, byteTotal ) );
	CUT_CHECK_ERROR( "cudaMalloc failed.\n" );

	// Memcpy to linear memory
	CUDA_SAFE_CALL( cudaMemcpy( g_texNormalsData, normals, byteTotal, cudaMemcpyHostToDevice ) );
	CUT_CHECK_ERROR( "cudaMemcpy failed.\n" );

	// Bind the data to the texture
    CUDA_SAFE_CALL( cudaBindTexture( 0, texNormals, g_texNormalsData, byteTotal ) );
}

void cudaTransferCellPointers( int32* cellPointers, uint32 size )
{
	uint32 byteTotal = size*sizeof(int32);

	// Malloc linear memory
	CUDA_SAFE_CALL( cudaMalloc( (void**)&g_texCellPointersData, byteTotal ) );
	CUT_CHECK_ERROR( "cudaMalloc failed.\n" );

	// Memcpy to linear memory
	CUDA_SAFE_CALL( cudaMemcpy( g_texCellPointersData, cellPointers, byteTotal, cudaMemcpyHostToDevice ) );
	CUT_CHECK_ERROR( "cudaMemcpy failed.\n" );

	// Bind the data to the texture
    CUDA_SAFE_CALL( cudaBindTexture( 0, texCellPointers, g_texCellPointersData, byteTotal ) );
}

void cudaTransferCellTriangleIds( int32* cellTriangleIds, uint32 size )
{
	uint32 byteTotal = size*sizeof(int32);

	// Malloc linear memory
	CUDA_SAFE_CALL( cudaMalloc( (void**)&g_texCellTriangleIdsData, byteTotal ) );
	CUT_CHECK_ERROR( "cudaMalloc failed.\n" );

	// Memcpy to linear memory
	CUDA_SAFE_CALL( cudaMemcpy( g_texCellTriangleIdsData, cellTriangleIds, byteTotal, cudaMemcpyHostToDevice ) );
	CUT_CHECK_ERROR( "cudaMemcpy failed.\n" );

	// Bind the data to the texture
    CUDA_SAFE_CALL( cudaBindTexture( 0, texCellTriangleIds, g_texCellTriangleIdsData, byteTotal ) );
}

void cudaSetBoxMin( float x, float y, float z )
{
	g_grid.boxMin.x = x;
	g_grid.boxMin.y = y;
	g_grid.boxMin.z = z;
}

void cudaSetBoxMax( float x, float y, float z )
{
	g_grid.boxMax.x = x;
	g_grid.boxMax.y = y;
	g_grid.boxMax.z = z;
}

void cudaSetGridParameters( float cellSizeX, float cellSizeY, float cellSizeZ,
						    int32 numCellsX, int32 numCellsY, int32 numCellsZ )
{
	g_grid.cellSize.x = cellSizeX;
	g_grid.cellSize.y = cellSizeY;
	g_grid.cellSize.z = cellSizeZ;

	g_grid.invCellSize.x = 1.0f / cellSizeX;
	g_grid.invCellSize.y = 1.0f / cellSizeY;
	g_grid.invCellSize.z = 1.0f / cellSizeZ;

	g_grid.gridSize.x = numCellsX;
	g_grid.gridSize.y = numCellsY;
	g_grid.gridSize.z = numCellsZ;

	// TODO: 
	//unsigned int numElements = numCellsX*numCellsY*numCellsZ;
	//unsigned int memSize = numElements * sizeof(float);
	//CUDA_SAFE_CALL( cudaMalloc( (void**)&g_triangleCounters, memSize ) );
	//CUT_CHECK_ERROR( "cudaMalloc failed.\n" );

	//CUDA_SAFE_CALL( cudaMalloc( (void**)&g_triangleIndices, memSize ) );
	//CUT_CHECK_ERROR( "cudaMalloc failed.\n" );

 //   g_scanConfig.direction      = CUDPP_SCAN_FORWARD;
 //   g_scanConfig.exclusivity    = CUDPP_SCAN_EXCLUSIVE;
 //   g_scanConfig.op		        = CUDPP_ADD;
 //   g_scanConfig.datatype       = CUDPP_FLOAT;
 //   g_scanConfig.maxNumElements = numElements;
 //   g_scanConfig.maxNumRows	    = 1;
 //   g_scanConfig.rowPitch       = 0;

 //   cudppInitializeScan( &g_scanConfig );
}

void cudaSetBlockSize( uint32 width, uint32 height )
{
	//g_blockWidth = width;
	//g_blockHeight = height;
}

void cudaRayTrace()
{
	// Map PBO to CUDA
	CUDA_SAFE_CALL( cudaGLMapBufferObject( (void**)&g_camera.frameBuffer, g_frameBufferId ) );

	// Setup execution configuration
	dim3 blockSize( g_blockWidth, g_blockHeight );
	dim3 gridSize( g_camera.screenWidth/g_blockWidth, g_camera.screenHeight/g_blockHeight, 1 );

	//blockSize.x = g_blockWidth;
	//blockSize.y = g_blockHeight;

	//int cellTotal = g_grid.gridSize.x * g_grid.gridSize.y * g_grid.gridSize.z;
	//gridSize.x = ceilf( cellTotal / (float)blockSize.x );

	// Call kernel
	//countTriangles<<<gridSize, blockSize>>>( g_vertexCount, g_grid, g_triangleCounters );
	//cudppScan( g_triangleIndices, g_triangleCounters, cellTotal, &g_scanConfig );

	rayTrace<<<gridSize, blockSize>>>( g_camera, g_grid );

	// Check for launch failure
	CUT_CHECK_ERROR( "Ray tracing kernel execution failed.\n" );

	// Unmap PBO back to OpenGL
	CUDA_SAFE_CALL( cudaGLUnmapBufferObject( g_frameBufferId ) );
}

} // extern "C"
