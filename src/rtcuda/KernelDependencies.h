#ifndef _RTC_KERNELDEPENDENCIES_H_
#define _RTC_KERNELDEPENDENCIES_H_

#include "CudaMath.h"

//////////////////////////////////////////////////////////////////////////
// Main definitions
//////////////////////////////////////////////////////////////////////////
static const unsigned int g_blockWidth  = 16;
static const unsigned int g_blockHeight = 12;

//////////////////////////////////////////////////////////////////////////
// Main structures
//////////////////////////////////////////////////////////////////////////
struct Camera
{
	uchar4* frameBuffer;
	unsigned int screenWidth;
	unsigned int screenHeight;

	float invScreenWidth;
	float invScreenHeight;

	float3 position;
	float3 baseDir;
	float3 nearU;
	float3 nearV;
};

struct Grid
{
	inline __device__ float3 worldToVoxel( float3 value )
	{
		return ( value - boxMin ) * invCellSize;
	}

	inline __device__ float3 voxelToWorld( float3 voxel )
	{
		return voxel * cellSize + boxMin;
	}

	inline __device__ int to1dCoord( float3 coords )
	{
		return coords.x + coords.y*gridSize.x + coords.z*gridSize.x*gridSize.y;
	}

	inline __device__ float3 to3dCoord( int idx )
	{
		float3 cellCoords;

		cellCoords.z = floor( (float)idx / (gridSize.x * gridSize.y) );
		cellCoords.y = floor( ((float)idx - cellCoords.z * gridSize.x * gridSize.y) / gridSize.x );
		cellCoords.x = floor( ((float)idx - cellCoords.z * gridSize.x * gridSize.y - cellCoords.y * gridSize.x) );

		return cellCoords;
	}

	// Grid AABB
	float3 boxMin;
	float3 boxMax;

	// Cell width, height and depth
	float3 cellSize;
	float3 invCellSize;

	// Number of cells in each dimension
	float3 gridSize;
};

struct Ray
{
	float3 orig;
	float3 dir;
	float3 invDir;

	float tnear;
	float tfar;
};

struct Hit
{
	int id; // triangle id
	float u;
	float v;
	float dist;
};

#endif // _RTC_KERNELDEPENDENCIES_H_
