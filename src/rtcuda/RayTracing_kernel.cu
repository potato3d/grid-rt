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
static inline __device__ float3 tex1Dfetch3( texture<float4, 1, cudaReadModeElementType> texRef, int idx )
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

	/************************************************************************/
	/* Initial setup                                                        */
	/************************************************************************/
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

	/************************************************************************/
	/* Trace ray through grid                                               */
	/************************************************************************/
	// Find first non-empty cell
	int2 cell = tex1Dfetch( texCellPointers, grid.to1dCoord( cellCoords ) );

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
		cell = tex1Dfetch( texCellPointers, grid.to1dCoord( cellCoords ) );

	} while( cellCoords.x != outLimit.x && cellCoords.y != outLimit.y && cellCoords.z != outLimit.z );

	// Background color
	camera.frameBuffer[pixelAddress] = make_color( 255 );
}

/*

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Triangle-ABB Overlap Axis Tests
// These were defines in original algorithm
static inline __device__ void axisTestX( const float3& boxHalfSizes, const float3& vA, const float3& vB, 
	                                     float a, float b, float fa, float fb, float& minp, float& maxp, float& rad )
{
	const float p0 = a*vA.y - b*vA.z;
	const float p2 = a*vB.y - b*vB.z;

	if( p0 < p2 )
	{
		minp=p0;
		maxp=p2;
	}
	else
	{
		minp=p2;
		maxp=p0;
	}

	rad = fa * boxHalfSizes.y + fb * boxHalfSizes.z;
}

static inline __device__ void axisTestY( const float3& boxHalfSizes, const float3& vA, const float3& vB, 
	                                     float a, float b, float fa, float fb, float& minp, float& maxp, float& rad )
{
	const float p0 = -a*vA.x + b*vA.z;
	const float p2 = -a*vB.x + b*vB.z;

	if( p0 < p2 )
	{
		minp = p0;
		maxp = p2;
	}
	else
	{
		minp = p2;
		maxp = p0;
	}

	rad = fa * boxHalfSizes.x + fb * boxHalfSizes.z;
}

static inline __device__ void axisTestZ( const float3& boxHalfSizes, const float3& vA, const float3& vB, 
	                                     float a, float b, float fa, float fb, float& minp, float& maxp, float& rad )
{
	const float p1 = a*vA.x - b*vA.y;
	const float p2 = a*vB.x - b*vB.y;

	if( p2 < p1 )
	{
		minp = p2;
		maxp = p1;
	}
	else
	{
		minp = p1;
		maxp = p2;
	}

	rad = fa * boxHalfSizes.x + fb * boxHalfSizes.y;
}

static inline __device__ void findMinMax( float a, float b, float c, float& minp, float& maxp )
{
	minp = a;
	maxp = a;

	if( b < minp )
		minp = b;
	if( b > maxp )
		maxp = b;
	if( c < minp )
		minp = c;
	if( c > maxp )
		maxp = c;
}

static inline __device__ bool planeBoxOverlap( const float3& boxHalfSizes, const float3& normal, 
	                                           const float3& vertex )
{
	float3 vmin;
	float3 vmax;

	//////////////////////////////////////////////////////
	// X
	float v = vertex.x;					// -NJMP-
	if( normal.x > 0.0f )
	{
		vmin.x = -boxHalfSizes.x - v;	// -NJMP-
		vmax.x =  boxHalfSizes.x - v;	// -NJMP-
	}
	else
	{
		vmin.x =  boxHalfSizes.x - v;	// -NJMP-
		vmax.x = -boxHalfSizes.x - v;	// -NJMP-
	}

	//////////////////////////////////////////////////////
	// Y
	v = vertex.y;					// -NJMP-
	if( normal.y > 0.0f )
	{
		vmin.y = -boxHalfSizes.y - v;	// -NJMP-
		vmax.y =  boxHalfSizes.y - v;	// -NJMP-
	}
	else
	{
		vmin.y =  boxHalfSizes.y - v;	// -NJMP-
		vmax.y = -boxHalfSizes.y - v;	// -NJMP-
	}

	//////////////////////////////////////////////////////
	// Z
	v = vertex.z;					// -NJMP-
	if( normal.z > 0.0f )
	{
		vmin.z = -boxHalfSizes.z - v;	// -NJMP-
		vmax.z =  boxHalfSizes.z - v;	// -NJMP-
	}
	else
	{
		vmin.z =  boxHalfSizes.z - v;	// -NJMP-
		vmax.z = -boxHalfSizes.z - v;	// -NJMP-
	}

	if( dot( normal, vmin ) > 0.0f ) // -NJMP-
		return false;

	if( dot( normal, vmax ) >= 0.0f ) // -NJMP-
		return true;

	return false;
}

static inline __device__ bool overlaps( float3 minv, float3 maxv, float3 v0, float3 v1, float3 v2 )
{
// Tomas Akenine-Möller Aabb-triangle overlap test (author's optimized and fixed version 18-06-2001)
	// http://www.cs.lth.se/home/Tomas_Akenine_Moller/code/tribox3.txt

	// Use separating axis theorem to test overlap between triangle and box
	// Need to test for overlap in these cases (directions):
	// 1) the {x,y,z}-directions (actually, since we use the Aabb of the triangle we do not even need to test these)
	// 2) normal of the triangle
	// 3) crossproduct(edge from tri, {x,y,z}-direction), this gives 3x3=9 more tests

	// Compute box center and halfsizes
	float3 boxCenter = ( maxv + minv ) * 0.5f;
	float3 boxHalfSizes = ( maxv - minv ) * 0.5f;

	// This is the fastest branch on Sun
	// Move everything so that the boxcenter is in (0,0,0)
	float3 vv0 = v0 - boxCenter;
	float3 vv1 = v1 - boxCenter;
	float3 vv2 = v2 - boxCenter;

	// Compute triangle edges
	float3 e0 = vv1 - vv0;
	float3 e1 = vv2 - vv1;
	float3 e2 = vv0 - vv2;

	// Case 3: test the 9 tests first (this was faster)
	float minp;
	float maxp;
	float rad;

	// Edge 0
	float fex = abs( e0.x );
	float fey = abs( e0.y );
	float fez = abs( e0.z );

	axisTestX( boxHalfSizes, vv0, vv2, e0.z, e0.y, fez, fey, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestY( boxHalfSizes, vv0, vv2, e0.z, e0.x, fez, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestZ( boxHalfSizes, vv1, vv2, e0.y, e0.x, fey, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	// Edge 1
	fex = abs( e1.x );
	fey = abs( e1.y );
	fez = abs( e1.z );

	axisTestX( boxHalfSizes, vv0, vv2, e1.z, e1.y, fez, fey, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestY( boxHalfSizes, vv0, vv2, e1.z, e1.x, fez, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestZ( boxHalfSizes, vv0, vv1, e1.y, e1.x, fey, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	// Edge 2
	fex = abs( e2.x );
	fey = abs( e2.y );
	fez = abs( e2.z );

	axisTestX( boxHalfSizes, vv0, vv1, e2.z, e2.y, fez, fey, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestY( boxHalfSizes, vv0, vv1, e2.z, e2.x, fez, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	axisTestZ( boxHalfSizes, vv1, vv2, e2.y, e2.x, fey, fex, minp, maxp, rad );
	if( minp > rad || maxp < -rad )
		return false;

	// Case 1: first test overlap in the {x,y,z}-directions
	// Find min, max of the triangle each direction, and test for overlap in that direction 
	// this is equivalent to testing a minimal Aabb around the triangle against the Aabb

	// Test in X-direction
	findMinMax( vv0.x, vv1.x, vv2.x, minp, maxp );
	if( minp > boxHalfSizes.x || maxp < -boxHalfSizes.x )
		return false;

	// Test in Y-direction
	findMinMax( vv0.y, vv1.y, vv2.y, minp, maxp );
	if( minp > boxHalfSizes.y || maxp < -boxHalfSizes.y )
		return false;

	// Test in Z-direction
	findMinMax( vv0.z, vv1.z, vv2.z, minp, maxp );
	if( minp > boxHalfSizes.z || maxp < -boxHalfSizes.z )
		return false;

	// Case 2: test if the box intersects the plane of the triangle
	// compute plane equation of triangle: normal*x+d=0
	float3 normal = cross( e0, e1 );

	// -NJMP- (line removed here)

	if( !planeBoxOverlap( boxHalfSizes, normal, vv0 ) ) // -NJMP-
		return false;

	return true;
}

__global__ void countTriangles( unsigned int vertexCount, Grid grid, float* output )
{
	//////////////////////////////////////////////////////////////////////////
	// Step 1: compute cell 1D and 3D coordinates
	//////////////////////////////////////////////////////////////////////////
	int cellIdx = blockIdx.x * blockDim.x + threadIdx.x;

	// Skip padding threads
	if( cellIdx >= grid.gridSize.x * grid.gridSize.y * grid.gridSize.z )
		return;

	float3 cellCoords = grid.to3dCoord( cellIdx );

	//////////////////////////////////////////////////////////////////////////
	// Step 2: iterate through all triangles and count how many intersect this cell
	//////////////////////////////////////////////////////////////////////////
	float3 cellMinv = grid.boxMin + cellCoords * grid.cellSize;
	float3 cellMaxv = cellMinv + grid.cellSize;

	// Output counter
	float numOverlapTriangles = 0.0f;

__shared__ float3 s_triVertices[g_blockWidth];

#ifdef __DEVICE_EMULATION__
	#define s_vertex(i) CUT_BANK_CHECKER( s_triVertices, i )
#else
	#define s_vertex(i) s_triVertices[i]
#endif

	// For each triangle, check if it overlaps current cell
	for( unsigned int v = 0; v < vertexCount; v+=g_blockWidth )
	{
		// Each thread loads a single vertex to shared memory
		s_vertex(threadIdx.x) = tex1Dfetch3( texVertices, v + threadIdx.x );

		// Sync before processing shared memory
		__syncthreads();

		// Loop over shared vertices
		for( unsigned int i = 0; i < g_blockWidth; i+=3 )
		{
			float3 v0 = s_vertex(i);
			float3 v1 = s_vertex(i+1);
			float3 v2 = s_vertex(i+2);

			if( overlaps( cellMinv, cellMaxv, v0, v1, v2 ) )
				++numOverlapTriangles;
		}

		// Sync before loading another batch of vertices to shared memory
        __syncthreads();
	}

	// For each triangle, check if it overlaps current cell
	//for( unsigned int t = 0; t < vertexCount; t+=3 )
	//{
	//	// Get triangle vertices
	//	//float3 v0 = make_float3( t+1, t, t );
	//	//float3 v1 = make_float3( t, t, t+1 );
	//	//float3 v2 = make_float3( t, t+1, t );

	//	float3 v0 = tex1Dfetch3( texVertices, t );
	//	float3 v1 = tex1Dfetch3( texVertices, t+1 );
	//	float3 v2 = tex1Dfetch3( texVertices, t+2 );

	//	//float3 triMinv = min( v0, min( v1, v2 ) );
	//	//float3 triMaxv = max( v0, max( v1, v2 ) );

	//	//float3 triCellStart = floor( clamp( grid.worldToVoxel( triMinv ), 
	//	//                                    make_float3( 0.0f ), grid.gridSize - make_float3( 1.0f ) ) );

	//	//float3 triCellEnd = floor( clamp( grid.worldToVoxel( triMaxv ), 
	//	//                                    make_float3( 0.0f ), grid.gridSize - make_float3( 1.0f ) ) );

	//	//bool overlap = ( triCellStart.x <= cellCoords.x ) && ( cellCoords.x <= triCellEnd.x ) &&
	//	//               ( triCellStart.y <= cellCoords.y ) && ( cellCoords.y <= triCellEnd.y ) &&
	//	//               ( triCellStart.z <= cellCoords.z ) && ( cellCoords.z <= triCellEnd.z );

	//	//if( overlap )
	//	//	++numOverlapTriangles;

	//	if( overlaps( cellMinv, cellMaxv, v0, v1, v2 ) )
	//	//float3 v = v0 + v1 + v2;
	//	//if( v.x > 0 && v.y > 0 && v.z > 0 )
	//		++numOverlapTriangles;
	//}

	//////////////////////////////////////////////////////////////////////////
	// Step 3: write number of overlapping triangles in output
	//////////////////////////////////////////////////////////////////////////
	output[cellIdx] = numOverlapTriangles;
}
*/
