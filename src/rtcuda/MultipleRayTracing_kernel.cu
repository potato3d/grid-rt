/*

#include "KernelDependencies.h"


// TODO: teste collecting cells first and intersecting later. reduces registers, 
// but without mem coalescing it is actually 1/3 original performance (300fps for 1 triangle)

// Kernel dependencies
#include "KernelDependencies.h"

// Global constants
static __device__ const float EPSILON   = 2e-6f;
static __device__ const float MAX_VALUE = 1e20f;

// Grid data
texture<int2, 1> texCellPointers;
texture<int, 1>  texCellTriangleIds;

// Geometry data
texture<float4, 1>  texVertices;
texture<float4, 1>  texNormals;

// 3-component texture fetch
template<class Type>
static inline __device__ float3 tex1Dfetch3( texture<Type, 1, cudaReadModeElementType> texRef, int idx )
{
	float4 texel = tex1Dfetch( texRef, idx );
	return make_float3( texel.x, texel.y, texel.z );
}

// Color conversion functions
static inline __device__ uchar4 make_color( unsigned char r, unsigned char g, unsigned char b )
{
    return make_uchar4( r, g, b, 255 );
}

static inline __device__ uchar4 make_color( unsigned char value )
{
    return make_uchar4( value, value, value, 255 );
}

static inline __device__ uchar4 make_color( float3 color, float alpha )
{
    return make_uchar4( color.x * 255.0f, color.y * 255.0f, color.z * 255.0f, alpha * 255.0f );
}

// Clip ray against bounding box
static inline __device__ bool rayHitsBoundingBox( float3 boxMin, float3 boxMax, Ray& ray )
{
	float3 t1 = ( boxMin - ray.orig ) * ray.invDir;
	float3 t2 = ( boxMax - ray.orig ) * ray.invDir;
	
	float3 minT1T2 = min( t1, t2 );
	float3 maxT1T2 = max( t1, t2 );
	
	ray.tnear = max( max( minT1T2.x, minT1T2.y ), minT1T2.z );
	ray.tfar = min( min( maxT1T2.x, maxT1T2.y ), maxT1T2.z );

	return ray.tnear <= ray.tfar;
}

// Compute intersection between triangle and current ray
// Moller-Trumbore algorithm
static inline __device__ void hitMT( int triangleId, const Ray& ray, Hit& hit )
{
	float3 v0 = tex1Dfetch3( texVertices, triangleId );
	float3 v1 = tex1Dfetch3( texVertices, triangleId + 1 );
	float3 v2 = tex1Dfetch3( texVertices, triangleId + 2 );

	float3 e1 = v1 - v0;
	float3 e2 = v2 - v0;
	float3 tvec = ray.orig - v0;

	float3 p = cross(ray.dir, e2);
	float3 q = cross(tvec, e1);

	float invdet = 1.0f / dot(p, e1);

	float u = dot(p, tvec) * invdet;
	float v = dot(q, ray.dir) * invdet;

	// Update hit
	bool isHit = (u >= 0.0f) && (v >= 0.0f) && (u + v <= 1.0f);
	
	float t = dot(q, e2) * invdet;

	// Update hit
	isHit &= (t > 0.0f) && (t < ray.tfar + EPSILON) && (t < hit.dist);

	if( isHit )
	{
		hit.id = triangleId;
		hit.u = u;
		hit.v = v;
		hit.dist = t;
	}
}

// Compute interpolated shading normal
static inline __device__ float3 computeShadingNormal( const Hit& hit )
{
	// Get triangle normals
    float3 n0 = tex1Dfetch3( texNormals, hit.id );
    float3 n1 = tex1Dfetch3( texNormals, hit.id + 1 );
    float3 n2 = tex1Dfetch3( texNormals, hit.id + 2 );

	return normalize( n0 * ( 1.0f - ( hit.u + hit.v ) ) +	// v0 coord
		              n1 * hit.u +							// v1 coord
					  n2 * hit.v );							// v2 coord
}

static inline __device__ uchar4 shade( const Ray& ray, const Hit& hit )
{
	// Need normalized ray direction
	float3 rayDirNormalized = normalize( ray.dir );

	// Compute interpolated shading normal
	float3 sampleNormal = computeShadingNormal( hit );

	// Hard-coded material information
	const float3 ambient = make_float3( 0.1f, 0.1f, 0.1f );
	const float3 diffuse = make_float3( 0.0f, 0.0f, 1.0f );

	// Headlight illumination
	float nDotD = -dot( sampleNormal, rayDirNormalized );
	float3 sampleColor = ( ambient + ( diffuse - ambient ) * nDotD ) * diffuse;
	return make_color( sampleColor, 1.0f );
}

static inline __device__ int2 getCell( const Grid& grid, float3 coords )
{
	int linearCoord = coords.x + coords.y*grid.gridSize.x + coords.z*grid.gridSize.x*grid.gridSize.y;
	return tex1Dfetch( texCellPointers, linearCoord );
}

static inline __device__ bool isEmpty( int2 cell )
{
	return cell.y == 0;
}

static inline __device__ int getTriangleStart( int2 cell )
{
	return cell.x;
}

static inline __device__ int getTriangleCount( int2 cell )
{
	return cell.y;
}

// Kernel for entire ray tracing pipeline
// Each thread traces a single ray
// Each thread computes its own ray direction
__global__ void rayTrace( Camera camera, Grid grid )
{
	//////////////////////////////////////////////////////////////////////////
	// Step 1: compute my screen coordinates
	//////////////////////////////////////////////////////////////////////////
	unsigned int screenX = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int screenY = blockIdx.y * blockDim.y + threadIdx.y;

	float uStep = (float)screenX * camera.invScreenWidth;
	float vStep = (float)screenY * camera.invScreenHeight;

	unsigned int pixelAddress = screenY*camera.screenWidth + screenX;

	//////////////////////////////////////////////////////////////////////////
	// Step 2.1: minimal ray attributes for box clipping
	//////////////////////////////////////////////////////////////////////////
	Ray ray;

	ray.orig = camera.position;

	ray.dir = camera.baseDir + camera.nearU*uStep + camera.nearV*vStep;
	ray.invDir = rcp( ray.dir );

    // If don't hit bbox in local space, no need to trace underlying triangles
	if( !rayHitsBoundingBox( grid.boxMin, grid.boxMax, ray ) )
	{
		// Background color
		camera.frameBuffer[pixelAddress] = make_color( 255 );
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	// Step 2.2: remaining ray attributes
	//////////////////////////////////////////////////////////////////////////
	float3 dirSignBits;
    dirSignBits.x = ( ray.dir.x < 0.0f )? 1.0f : 0.0f;
    dirSignBits.y = ( ray.dir.y < 0.0f )? 1.0f : 0.0f;
    dirSignBits.z = ( ray.dir.z < 0.0f )? 1.0f : 0.0f;

	float3 notDirSignBits = not( dirSignBits );

	//////////////////////////////////////////////////////////////////////////
	// Initial setup                                                        
	//////////////////////////////////////////////////////////////////////////
	// 1. Find initial cell where ray begins

	// Since ray was already clipped against bbox (grid),
	// ray tnear gives us the starting t (thus the start point as well)
	float3 startPoint = ray.orig + ray.dir * ray.tnear;
	
	// Floor is needed when working with float values (equivalent to truncating to int)
	float3 cellCoords = floor( clamp( grid.worldToVoxel( startPoint ), 
		                              make_float3( 0.0f ), grid.gridSize - make_float3( 1.0f ) ) );

	// 2. Compute stepX, stepY, stepZ
	float3 cellStep = -dirSignBits + notDirSignBits;

	// 3 Compute out of grid limits
	float3 outLimit = -dirSignBits + grid.gridSize * notDirSignBits;

	// 4. Compute tDeltaX, tDeltaY, tDeltaZ
	float3 tDelta = abs( grid.cellSize * ray.invDir );

	// 5. Compute tNextX, tNextY, tNextZ
	float3 tMax = ( grid.voxelToWorld( cellCoords + notDirSignBits ) - ray.orig ) * ray.invDir;

	//////////////////////////////////////////////////////////////////////////
	// Trace ray through grid                                               
	//////////////////////////////////////////////////////////////////////////
	// Find first non-empty cell
	int2 cell = getCell( grid, cellCoords );

	// Store hit information
	Hit hit;
	hit.dist = MAX_VALUE;

	// Minimum tMax in all 3 dimensions, used for logical comparison to determine next cell
	float minTmax;

	// Stores 1 for next cell dimension and 0 for the others, used to select next cell
	float3 comp;

	// While inside grid
	do
	{
		// Early traversal pre-computation
		// Already begin computing next cell to be visited before testing current one
		// To go to next cell, need to decide which dimension is next
		// comp stores 1 for next dimension and 0 for others
 		minTmax = min( min( tMax.x, tMax.y ), tMax.z );
 		comp = step( tMax - make_float3( minTmax ), make_float3( EPSILON ) );

		// Step ray according to comp
 		cellCoords += cellStep * comp;
 		tMax += tDelta * comp;

		// If cell contains triangles, test intersection
		if( !isEmpty( cell ) )
		{
			// We send the smallest tMax as the maximum valid distance
			// This avoids false intersections outside current cell
			ray.tfar = minTmax;

			// Iterate through triangles in given cell and compute nearest intersection, if any
			int i = getTriangleStart( cell );
			int end = getTriangleStart( cell ) + getTriangleCount( cell );

			while( i < end )
			{
				// Get triangle id
				int triangleId = tex1Dfetch( texCellTriangleIds, i );

				// Check for intersection
				hitMT( triangleId, ray, hit );
		        
				// Go to next triangle
				++i;
			}

			// If found hit
			if( hit.dist < MAX_VALUE )
			{
				camera.frameBuffer[pixelAddress] = shade( ray, hit );
				return;
			}
		}

		// Get next cell
		// The above code could all go here, but it is faster to do it early
		cell = getCell( grid, cellCoords );

	} while( cellCoords.x != outLimit.x && cellCoords.y != outLimit.y && cellCoords.z != outLimit.z );

	// Background color
	camera.frameBuffer[pixelAddress] = make_color( 255 );
}

//////////////////////////////////////////////////////////////////////

// TODO: need methods from "RayTracing_kernel.cu"
// TODO: all goes well until traceValidRays kernel -> requires 40+ registers!!!

__global__ void initRays( Camera camera, float4* rayOrigins, float4* rayDirs, float4* rayInvDirs )
{
	//////////////////////////////////////////////////////////////////////////
	// Step 1: compute my screen coordinates
	//////////////////////////////////////////////////////////////////////////
	unsigned int screenX = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int screenY = blockIdx.y * blockDim.y + threadIdx.y;
	unsigned int pixelAddress = screenY*camera.screenWidth + screenX;

	//////////////////////////////////////////////////////////////////////////
	// Step 2: compute ray directions
	//////////////////////////////////////////////////////////////////////////
	float uStep = (float)screenX * camera.invScreenWidth;
	float vStep = (float)screenY * camera.invScreenHeight;
	float3 rayDir = camera.baseDir + camera.nearU*uStep + camera.nearV*vStep;

	//////////////////////////////////////////////////////////////////////////
	// Step 3: store values for next kernel
	//////////////////////////////////////////////////////////////////////////
	rayOrigins[pixelAddress] = make_float4( camera.position );
	rayDirs[pixelAddress] = make_float4( rayDir );
	rayInvDirs[pixelAddress] = make_float4( rcp( rayDir ) );
}

__global__ void hitSceneBox( Camera camera, Grid grid, 
							 float4* rayOrigins, float4* rayInvDirs, float* tnears )
{
	//////////////////////////////////////////////////////////////////////////
	// Step 1: compute my screen coordinates
	//////////////////////////////////////////////////////////////////////////
	unsigned int screenX = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int screenY = blockIdx.y * blockDim.y + threadIdx.y;
	unsigned int pixelAddress = screenY*camera.screenWidth + screenX;

	//////////////////////////////////////////////////////////////////////////
	// Step 2: minimal ray attributes for box clipping
	//////////////////////////////////////////////////////////////////////////
	float3 rayOrig   = make_float3( rayOrigins[pixelAddress] );
	float3 rayInvDir = make_float3( rayInvDirs[pixelAddress] );

	//////////////////////////////////////////////////////////////////////////
	// Step 3: clip ray against grid bounding box
	//////////////////////////////////////////////////////////////////////////
	float3 t1 = ( grid.boxMin - rayOrig ) * rayInvDir;
	float3 t2 = ( grid.boxMax - rayOrig ) * rayInvDir;
	
	float3 minT1T2 = min( t1, t2 );
	float3 maxT1T2 = max( t1, t2 );
	
	float tnear = max( max( minT1T2.x, minT1T2.y ), minT1T2.z );
	float tfar = min( min( maxT1T2.x, maxT1T2.y ), maxT1T2.z );

	bool hit = tnear <= tfar;

	//////////////////////////////////////////////////////////////////////////
	// Step 4: store values for next kernel
	//////////////////////////////////////////////////////////////////////////
	tnears[pixelAddress] = ( hit )? tnear : MAX_VALUE;
}

__global__ void traceValidRays( Camera camera, Grid grid, 
							    float4* rayOrigins, float4* rayDirs, float4* rayInvDirs, float* tnears, Hit* hits )
{
	//////////////////////////////////////////////////////////////////////////
	// Step 1: compute my screen coordinates
	//////////////////////////////////////////////////////////////////////////
	unsigned int screenX = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int screenY = blockIdx.y * blockDim.y + threadIdx.y;
	unsigned int pixelAddress = screenY*camera.screenWidth + screenX;

	//////////////////////////////////////////////////////////////////////////
	// Step 2: abort invalid rays
	//////////////////////////////////////////////////////////////////////////
	Ray ray;
	ray.tnear = tnears[pixelAddress];

	if( ray.tnear == MAX_VALUE )
	{
		// Background color
		camera.frameBuffer[pixelAddress] = make_color( 255 );
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	// Step 3: remaining ray attributes
	//////////////////////////////////////////////////////////////////////////
	ray.orig   = make_float3( rayOrigins[pixelAddress] );
	ray.dir    = make_float3( rayDirs[pixelAddress] );
	ray.invDir = make_float3( rayInvDirs[pixelAddress] );

	float3 dirSignBits;
    dirSignBits.x = ( ray.dir.x < 0.0f )? 1.0f : 0.0f;
    dirSignBits.y = ( ray.dir.y < 0.0f )? 1.0f : 0.0f;
    dirSignBits.z = ( ray.dir.z < 0.0f )? 1.0f : 0.0f;

	float3 notDirSignBits = not( dirSignBits );

	//////////////////////////////////////////////////////////////////////////
	// Initial setup                                                        
	//////////////////////////////////////////////////////////////////////////
	// 1. Find initial cell where ray begins

	// Since ray was already clipped against bbox (grid),
	// ray tnear gives us the starting t (thus the start point as well)
	float3 startPoint = ray.orig + ray.dir * ray.tnear;
	
	// Floor is needed when working with float values (equivalent to truncating to int)
	float3 cellCoords = floor( clamp( grid.worldToVoxel( startPoint ), 
		                              make_float3( 0.0f ), grid.cellTotal - make_float3( 1.0f ) ) );

	// 2. Compute stepX, stepY, stepZ
	float3 cellStep = -dirSignBits + notDirSignBits;

	// 3 Compute out of grid limits
	float3 outLimit = -dirSignBits + grid.cellTotal * notDirSignBits;

	// 4. Compute tDeltaX, tDeltaY, tDeltaZ
	float3 tDelta = abs( grid.cellSize * ray.invDir );

	// 5. Compute tNextX, tNextY, tNextZ
	float3 tMax = ( grid.voxelToWorld( cellCoords + notDirSignBits ) - ray.orig ) * ray.invDir;

	//////////////////////////////////////////////////////////////////////////
	// Trace ray through grid                                               
	//////////////////////////////////////////////////////////////////////////
	// Find first non-empty cell
	int2 cell = getCell( grid, cellCoords );

	// Store hit information
	Hit hit;
	hit.dist = MAX_VALUE;

	// Minimum tMax in all 3 dimensions, used for logical comparison to determine next cell
	float minTmax;

	// Stores 1 for next cell dimension and 0 for the others, used to select next cell
	float3 comp;

	// While inside grid
	do
	{
		// Early traversal pre-computation
		// Already begin computing next cell to be visited before testing current one
		// To go to next cell, need to decide which dimension is next
		// comp stores 1 for next dimension and 0 for others
 		minTmax = min( min( tMax.x, tMax.y ), tMax.z );
 		comp = step( tMax - make_float3( minTmax ), make_float3( EPSILON ) );

		// Step ray according to comp
 		cellCoords += cellStep * comp;
 		tMax += tDelta * comp;

		// If cell contains triangles, test intersection
		if( !isEmpty( cell ) )
		{
			// We send the smallest tMax as the maximum valid distance
			// This avoids false intersections outside current cell
			ray.tfar = minTmax;

			// Iterate through triangles in given cell and compute nearest intersection, if any
			int i = getTriangleStart( cell );
			int end = getTriangleStart( cell ) + getTriangleCount( cell );

			while( i < end )
			{
				// Get triangle id
				int triangleId = tex1Dfetch( texCellTriangleIds, i );

				// Check for intersection
				hitMT( triangleId, ray, hit );
		        
				// Go to next triangle
				++i;
			}

			// If found hit
			if( hit.dist < MAX_VALUE )
				break;
		}

		// Get next cell
		// The above code could all go here, but it is faster to do it early
		cell = getCell( grid, cellCoords );

	} while( cellCoords.x != outLimit.x && cellCoords.y != outLimit.y && cellCoords.z != outLimit.z );

	//////////////////////////////////////////////////////////////////////////
	// Step 4: store values for next kernel
	//////////////////////////////////////////////////////////////////////////
	hits[pixelAddress] = hit;
}

__global__ void shadeHits( Camera camera, float4* rayDirs, Hit* hits )
{
	//////////////////////////////////////////////////////////////////////////
	// Step 1: compute my screen coordinates
	//////////////////////////////////////////////////////////////////////////
	unsigned int screenX = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int screenY = blockIdx.y * blockDim.y + threadIdx.y;
	unsigned int pixelAddress = screenY*camera.screenWidth + screenX;

	//////////////////////////////////////////////////////////////////////////
	// Step 2: abort rays that hit nothing
	//////////////////////////////////////////////////////////////////////////
	Hit hit = hits[pixelAddress];

	if( hit.dist == MAX_VALUE )
	{
		camera.frameBuffer[pixelAddress] = make_color( 255 );
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	// Step 3: shade intersected rays
	//////////////////////////////////////////////////////////////////////////
	Ray ray;
	ray.dir = make_float3( rayDirs[pixelAddress] );

	camera.frameBuffer[pixelAddress] = shade( ray, hit );
}

*/
