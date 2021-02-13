#version 120
#extension GL_ARB_texture_rectangle : enable

// Reflection
uniform bool u_reflectionPass;
uniform sampler2DRect u_texHits;

// Ray data
uniform vec3 u_rayOrig;
varying vec3 v_rayDir;

struct Ray
{
	vec3 orig;
	vec3 dir;
	vec3 invDir;
	float tfar;
} g_ray;

vec4 g_hit;

// Scene data
uniform sampler2DRect u_texTriangleVertices;
uniform sampler2DRect u_texTriangleNormals;

// Grid data
uniform sampler2DRect u_texCellPointers;
uniform sampler2DRect u_texCellTriangleIds;
uniform vec3 u_boxMin;
uniform vec3 u_boxMax;
uniform vec3 u_cellSize;    // dimensions of each cell
uniform vec3 u_invCellSize;
uniform vec3 u_gridSize;    // number of cells in each dimension

// Global constants
const float EPSILON   = 1e-5;
const float MAX_VALUE = 1e20;

const float MAX_TEX_SIZE = 8192.0;
const float INV_MAX_TEX_SIZE = 0.0001220703125;

////////////////////////////////////////////////////////////
// Helper functions
float myMod( float x, float y, float invY )
{
	return x - y * floor( x * invY );
}

vec2 get2DCoord( float linearCoord, float w, float invW )
{
	return vec2( myMod( linearCoord, w, invW ), floor( linearCoord * invW ) );
}

vec4 getTexel2DRect( sampler2DRect texSampler, float linearCoord )
{
	return texture2DRect( texSampler, get2DCoord( linearCoord, MAX_TEX_SIZE, INV_MAX_TEX_SIZE ) );
}

////////////////////////////////////////////////////////////
// Grid functions
vec3 worldToVoxel( vec3 value )
{
    return ( value - u_boxMin ) * u_invCellSize;
}

vec3 voxelToWorld( vec3 voxel )
{
    return voxel * u_cellSize + u_boxMin;
}

vec2 gridCellAt( vec3 cellCoords )
{
	return getTexel2DRect( u_texCellPointers, cellCoords.x + cellCoords.y * u_gridSize.x + cellCoords.z * u_gridSize.x * u_gridSize.y ).ba;
}

bool isEmptyCell( vec2 cell )
{
    return cell.y == 0.0;
}

float getCellTriangleStart( vec2 cell )
{
    return cell.x;
}

float getCellTriangleCount( vec2 cell )
{
    return cell.y;
}

////////////////////////////////////////////////////////////
// Main functions

// Compute interpolated shading normal
vec3 computeShadingNormal()
{
	// Get triangle normals
	// triangle id's are sequential, but we have 3 normals per triangle
    vec3 n0 = getTexel2DRect( u_texTriangleNormals, g_hit.x * 3.0 ).rgb;
    vec3 n1 = getTexel2DRect( u_texTriangleNormals, g_hit.x * 3.0 + 1.0 ).rgb;
    vec3 n2 = getTexel2DRect( u_texTriangleNormals, g_hit.x * 3.0 + 2.0 ).rgb;

	return normalize( n0 * ( 1.0 - ( g_hit.y + g_hit.z ) ) +	// v0 coord
		              n1 * g_hit.y +							// v1 coord
					  n2 * g_hit.z );							// v2 coord
}

// Compute intersection between triangle and current ray
// Moller-Trumbore algorithm
void hitMT( float triangleId )
{
    // Get triangle vertices
	// triangle id's are sequential, but we have 3 vertices per triangle
    vec3 v0 = getTexel2DRect( u_texTriangleVertices, triangleId * 3.0 ).rgb;
    vec3 v1 = getTexel2DRect( u_texTriangleVertices, triangleId * 3.0 + 1.0 ).rgb;
    vec3 v2 = getTexel2DRect( u_texTriangleVertices, triangleId * 3.0 + 2.0 ).rgb;

	vec3 e1 = v1 - v0;
	vec3 e2 = v2 - v0;
	vec3 p = cross(g_ray.dir, e2);
	float det = dot(p, e1);

	float invdet = 1.0 / det;
	vec3 tvec = g_ray.orig - v0;
	vec3 q = cross(tvec, e1);
	float u = dot(p, tvec) * invdet;
	float v = dot(q, g_ray.dir) * invdet;
	float t = dot(q, e2) * invdet;
	
	bool isHit = (u >= 0.0) && (v >= 0.0) && 
			     (u + v <= 1.0) && 
                 (t > 0.0) &&
				 (t < g_ray.tfar) &&
			     (t < g_hit.w);

	g_hit = isHit? vec4( triangleId, u, v, t ) : g_hit;
}

// Iterate through triangles in given cell and compute nearest intersection, if any
void intersectTriangles( vec2 cell )
{
    float i = getCellTriangleStart( cell );
    float end = i + getCellTriangleCount( cell );
    
	do
    {
        // Get triangle id
		// Luminance_alpha = (cellId, triangleId)
		// Need * 3 because grid builder uses sequential triangleId's, while we will use triangleId to access vertices directly (3 vertices per triangle) 
		float triangleId = getTexel2DRect( u_texCellTriangleIds, i ).a;

        // Check for intersection
        hitMT( triangleId );
        
		// Go to next triangle
        ++i;
    }
    while( i < end );
}

bool findNearestHit()
{
	// Start inverse of ray dir as early as possible
    g_ray.invDir = vec3( 1.0 ) / g_ray.dir;

    // If don't hit bbox in local space, no need to trace underlying triangles
	vec3 t1 = ( u_boxMin - g_ray.orig ) * g_ray.invDir;
	vec3 t2 = ( u_boxMax - g_ray.orig ) * g_ray.invDir;
	
	vec3 minT1T2 = min( t1, t2 );
	vec3 maxT1T2 = max( t1, t2 );
	
	float tnear = max( max( minT1T2.x, minT1T2.y ), minT1T2.z );
	float tmax = min( min( maxT1T2.x, maxT1T2.y ), maxT1T2.z );

	if( tnear > tmax )
		return false;

	// TODO: optimize, use language functions, etc
    vec3 dirSignBits;
    dirSignBits.x = ( g_ray.dir.x < 0.0 )? 1.0 : 0.0;
    dirSignBits.y = ( g_ray.dir.y < 0.0 )? 1.0 : 0.0;
    dirSignBits.z = ( g_ray.dir.z < 0.0 )? 1.0 : 0.0;

	vec3 notDirSignBits;
	notDirSignBits.x = ( g_ray.dir.x < 0.0 )? 0.0 : 1.0;
    notDirSignBits.y = ( g_ray.dir.y < 0.0 )? 0.0 : 1.0;
    notDirSignBits.z = ( g_ray.dir.z < 0.0 )? 0.0 : 1.0;
		
    /************************************************************************/
	/* Initial setup                                                        */
	/************************************************************************/
	// 1. Find initial cell where ray begins

	// Since ray was already clipped against grid bbox, tnear gives us the starting t (thus the start point as well)
	vec3 startPoint = g_ray.orig + g_ray.dir * tnear;
	
	// TODO: check if this is the best solution
	// floor is needed when working with float values (equivalent to truncating to int)
	vec3 cellCoords = floor( clamp( worldToVoxel( startPoint ), vec3( 0.0 ), u_gridSize - vec3( 1.0 ) ) );

	// 2. Compute rayStepX, rayStepY, rayStepZ
	vec3 rayStep = -dirSignBits + notDirSignBits;

	// TODO: check if it is faster to use logic operations or to branch based on ray.dir signs for each dimension

	// 3 Compute out of grid limits
	vec3 outLimit = -dirSignBits + u_gridSize * notDirSignBits;
	
	// 4. Compute tDeltaX, tDeltaY, tDeltaZ
	vec3 tDelta = abs( u_cellSize * g_ray.invDir );

	// 5. Compute tNextX, tNextY, tNextZ
	vec3 tMax = ( voxelToWorld( cellCoords + notDirSignBits ) - g_ray.orig ) * g_ray.invDir;
	
	/************************************************************************/
	/* Trace ray through grid                                               */
	/************************************************************************/
	// Find first non-empty cell
	vec2 cell;

	// Minimum tMax in all 3 dimensions, used for logical comparison to determine next cell
	float minTmax;

	// Stores 1 for next cell dimension and 0 for the others, used to select next cell
	vec3 comp;

	// Reset hit distance
	g_hit.w = MAX_VALUE;

	// While inside grid
	do
	{
		// Get next cell
		cell = gridCellAt( cellCoords );

		// Early traversal pre-computation
		// Already begin computing next cell to be visited before testing current one
		// To go to next cell, need to decide which dimension is next
		// comp stores 1 for next dimension and 0 for others
 		minTmax = min( min( tMax.x, tMax.y ), tMax.z );
 		comp = step( tMax - vec3( minTmax ), vec3( EPSILON ) );

		// Step ray according to comp
 		cellCoords += rayStep * comp;
 		tMax += tDelta * comp;

		// If cell contains triangles, test intersection
		if( !isEmptyCell( cell ) )
		{
			// We send the smallest tMax as the maximum valid distance
			// This avoids false intersections outside current cell
			g_ray.tfar = minTmax;
			intersectTriangles( cell );

			if( g_hit.w < MAX_VALUE )
				return true;
		}
	}
	while( cellCoords.x != outLimit.x && cellCoords.y != outLimit.y && cellCoords.z != outLimit.z );
	
	return false;
}

void main()
{
	if( !u_reflectionPass )
	{
		// Setup primary ray
		g_ray.orig = u_rayOrig;
		g_ray.dir = v_rayDir;
	}
	else
	{
		// Recover hit information
		g_hit = texture2DRect( u_texHits, gl_FragCoord.xy );

		// If no hit, skip reflection
		if( g_hit.x == MAX_VALUE )
		{
			gl_FragColor = g_hit;
			return;
		}

		// In case it was a hit in shadow
		g_hit.x = abs( g_hit.x );

		// Compute hit position
		vec3 hitPos = u_rayOrig + v_rayDir * g_hit.w;

		// Compute shading normal
		vec3 hitNormal = computeShadingNormal();

		// Setup reflection ray
		g_ray.orig = hitPos + hitNormal * 1e-3;
		g_ray.dir = reflect( v_rayDir, hitNormal );
	}
	
	if( findNearestHit() )
		gl_FragColor = g_hit;
	else
		gl_FragColor = vec4( MAX_VALUE );
}
