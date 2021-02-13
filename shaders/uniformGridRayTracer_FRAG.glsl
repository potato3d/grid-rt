#version 120
#extension GL_ARB_texture_rectangle : enable
#extension GL_EXT_gpu_shader4 : enable

// Shading data
const vec3 g_background = vec3( 1.0 );
const vec3 g_ambient  = vec3( 0.0 );
const vec3 g_specular = vec3( 0.5 );//1.0 );
const float g_specularExp = 50.0;//75.0;
const float g_primaryCoeff = 1.0;
const float g_shadowCoeff = 0.2;
const float g_reflexCoeff = 0.4;//0.2;
const float g_refractCoeff = 0.5;
const float g_refractIndex = 1.2;

// Lights
//const vec3 g_lightPosition = vec3( 1e10, 1e10, 1e10 );
//const vec3 g_lightPosition = vec3( 1e0, -1e10, -1e10 );
// monoBR: const vec3 g_lightPosition = vec3( -1e5, 1e10, 1e10 );
//const vec3 g_lightPosition = vec3( 1e10, 5e9, 1e10 );
// forest: const vec3 g_lightPosition = vec3( 0.5e0, 3e0, 2e0 );
// toys: const vec3 g_lightPosition = vec3( 3e9, 3e9, 1e10 );
// marbles: const vec3 g_lightPosition = vec3( 1e0, -1e10, 1e10 );
// ben: const vec3 g_lightPosition = vec3( 1e10, 1e0, 1e10 );
// wooddoll: const vec3 g_lightPosition = vec3( 1e3, -1e5, 1e10 );
// hand: const vec3 g_lightPosition = vec3( 1e0, -1e10, 1e10 );
const vec3 g_lightPosition = vec3( 1e3, -1e5, 1e10 );
const vec3 g_lightColor = vec3( 1.0, 1.0, 1.0 );

// Enable/disable effects
#define ENABLE_SHADING					0
#define ENABLE_TEXTURE					1
#define ENABLE_SHADOW					1
#define ENABLE_REFLECTION				0
#define REFLECT_BACKGROUND				0
#define OVERRIDE_COLOR					0

// not used
//#define ENABLE_ALL_REFLECTIONS		0
//#define ENABLE_REFRACTION				0

// Global constants
const float EPSILON   = 1e-5;
const float MAX_VALUE = 1e20;

const float MAX_TEX_SIZE = 8192.0;
const float INV_MAX_TEX_SIZE = 0.0001220703125;

// Texture access information
uniform sampler2DRect  u_texMaterials;
uniform sampler2DArray u_texMaterialTextureArray;

// Scene data
uniform sampler2DRect u_texTriangleVertices;
uniform sampler2DRect u_texTriangleNormals;
uniform sampler2DRect u_texTriangleTexcoords;

// Grid data
uniform sampler2DRect u_texCellPointers;
uniform sampler2DRect u_texCellTriangleIds;
uniform vec3 u_boxMin;
uniform vec3 u_boxMax;
uniform vec3 u_cellSize;    // dimensions of each cell
uniform vec3 u_invCellSize;
uniform vec3 u_gridSize;    // number of cells in each dimension

// Ray data
uniform vec3 u_rayOrig;
varying vec3 v_rayDir;
vec3 g_rayOrig;   // allow writing of ray origin to trace secondary rays
vec3 g_rayDir;
vec3 g_rayInvDir; // computed in the beginning of frag shader, since it depends on current ray

// Hit data
struct Hit
{
	float triangleId;
	float u;
	float v;
	float distance;
} g_hit;

// Sample data
vec3 g_hitPos;
vec3 g_hitNormal;


// Helper functions
float myMod( float x, float y, float invY )
{
	return x - y * floor( x * invY );
}

vec2 get2DCoord( float linearCoord, float w, float invW )
{
	return vec2( myMod( linearCoord, w, invW ), floor( linearCoord * invW ) );
}

vec4 getTexel2D( sampler2D texSampler, float linearCoord, float invTexHeight )
{
	vec2 coord;
	coord.x = myMod( linearCoord, MAX_TEX_SIZE, INV_MAX_TEX_SIZE ) * INV_MAX_TEX_SIZE; // normalize
	coord.y = linearCoord * INV_MAX_TEX_SIZE * invTexHeight; // normalize
	return texture2D( texSampler, coord );
}

vec4 getTexel2DRect( sampler2DRect texSampler, float linearCoord )
{
	return texture2DRect( texSampler, get2DCoord( linearCoord, MAX_TEX_SIZE, INV_MAX_TEX_SIZE ) );
}

// Grid helper functions
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

// Clip current ray against bounding box
bool rayHitsBoundingBox( inout float tnear )
{
	vec3 t1 = ( u_boxMin - g_rayOrig ) * g_rayInvDir;
	vec3 t2 = ( u_boxMax - g_rayOrig ) * g_rayInvDir;
	
	vec3 minT1T2 = min( t1, t2 );
	vec3 maxT1T2 = max( t1, t2 );
	
	     tnear = max( max( minT1T2.x, minT1T2.y ), minT1T2.z );
	float tmax = min( min( maxT1T2.x, maxT1T2.y ), maxT1T2.z );

	return tnear <= tmax;
}

// Compute intersection between triangle and current ray
// Moller-Trumbore algorithm
void hitMT( float triangleId, float tfar )
{
    // Get triangle vertices
	// triangle id's are sequential, but we have 3 vertices per triangle
    vec3 v0 = getTexel2DRect( u_texTriangleVertices, triangleId * 3.0 ).rgb;
    vec3 v1 = getTexel2DRect( u_texTriangleVertices, triangleId * 3.0 + 1.0 ).rgb;
    vec3 v2 = getTexel2DRect( u_texTriangleVertices, triangleId * 3.0 + 2.0 ).rgb;

	vec3 e1 = v1 - v0;
	vec3 e2 = v2 - v0;
	vec3 p = cross(g_rayDir, e2);
	float det = dot(p, e1);

	float invdet = 1.0 / det;
	vec3 tvec = g_rayOrig - v0;
	vec3 q = cross(tvec, e1);
	float u = dot(p, tvec) * invdet;
	float v = dot(q, g_rayDir) * invdet;
	float t = dot(q, e2) * invdet;
	
	bool isHit = (u >= 0.0) && (v >= 0.0) && 
			     (u + v <= 1.0) && 
                 (t > 0.0) &&
				 (t < tfar) &&
			     (t < g_hit.distance);

	g_hit = isHit? Hit( triangleId, u, v, t ) : g_hit;
}

// Iterate through triangles in given cell and compute nearest intersection, if any
void intersectTriangles( vec2 cell, float tfar )
{
    float i = getCellTriangleStart( cell );
    float end = i + getCellTriangleCount( cell );
    
    while( i < end )
    {
        // Get triangle id
		// Luminance_alpha = (cellId, triangleId)
		// Need * 3 because grid builder uses sequential triangleId's, while we will use triangleId to access vertices directly (3 vertices per triangle) 
		float triangleId = getTexel2DRect( u_texCellTriangleIds, i ).a;

        // Check for intersection
        hitMT( triangleId, tfar );
        
		// Go to next triangle
        ++i;
    }
}

// Compute interpolated shading normal
vec3 computeShadingNormal()
{
	// Get triangle normals
	// triangle id's are sequential, but we have 3 normals per triangle
    vec3 n0 = getTexel2DRect( u_texTriangleNormals, g_hit.triangleId * 3.0 ).rgb;
    vec3 n1 = getTexel2DRect( u_texTriangleNormals, g_hit.triangleId * 3.0 + 1.0 ).rgb;
    vec3 n2 = getTexel2DRect( u_texTriangleNormals, g_hit.triangleId * 3.0 + 2.0 ).rgb;

	return normalize( n0 * ( 1.0 - ( g_hit.u + g_hit.v ) ) +	// v0 coord
		              n1 * g_hit.u +							// v1 coord
					  n2 * g_hit.v );							// v2 coord
}

vec3 getTextureColor( float textureId )
{
	// Get triangle texcoords
	// triangle id's are sequential, but we have 3 texcoords per triangle
	// texture is luminance_alpha
    vec3 t0 = getTexel2DRect( u_texTriangleTexcoords, g_hit.triangleId * 3.0 ).raa;
    vec3 t1 = getTexel2DRect( u_texTriangleTexcoords, g_hit.triangleId * 3.0 + 1.0 ).raa;
    vec3 t2 = getTexel2DRect( u_texTriangleTexcoords, g_hit.triangleId * 3.0 + 2.0 ).raa;

	// TODO: for hand model
	//t0.y = 1.0 - t0.y;
	//t1.y = 1.0 - t1.y;
	//t2.y = 1.0 - t2.y;
	
	// TODO: for ben model
	//t0.x = 1.0 - t0.x;
	//t1.x = 1.0 - t1.x;
	//t2.x = 1.0 - t2.x;
	
	vec3 t = t0 * ( 1.0 - ( g_hit.u + g_hit.v ) ) +	// v0 coord
		     t1 * g_hit.u +						// v1 coord
			 t2 * g_hit.v;						// v2 coord

	// Use texture index to access texture array
	t.z = textureId;

	return texture2DArray( u_texMaterialTextureArray, t ).rgb;
}

// Compute hit position from current ray and hit attributes
void updateHitPositionAndNormal()
{
	g_hitPos = g_rayOrig + g_rayDir * g_hit.distance;
	g_hitNormal = computeShadingNormal();	
}

// 3D-DDA ray-grid traversal (Amanatides & Woo)
bool rayTrace()
{
	// Start inverse of ray dir as early as possible
    g_rayInvDir = vec3( 1.0 ) / g_rayDir;

	// Reset hit distance
	g_hit.distance = MAX_VALUE;

	// Ray entry point in bbox
	float tnear;
    
    // If don't hit bbox in local space, no need to trace underlying triangles
	if( !rayHitsBoundingBox( tnear ) )
		return false;

	// TODO: optimize, use language functions, etc
    vec3 dirSignBits;
    dirSignBits.x = ( g_rayDir.x < 0.0 )? 1.0 : 0.0;
    dirSignBits.y = ( g_rayDir.y < 0.0 )? 1.0 : 0.0;
    dirSignBits.z = ( g_rayDir.z < 0.0 )? 1.0 : 0.0;

	vec3 notDirSignBits = vec3( not( bvec3( dirSignBits ) ) );
		
    /************************************************************************/
	/* Initial setup                                                        */
	/************************************************************************/
	// 1. Find initial cell where ray begins

	// Since ray was already clipped against grid bbox, tnear gives us the starting t (thus the start point as well)
	vec3 startPoint = g_rayOrig + g_rayDir * tnear;
	
	// TODO: check if this is the best solution
	// floor is needed when working with float values (equivalent to truncating to int)
	vec3 cellCoords = floor( clamp( worldToVoxel( startPoint ), vec3( 0.0 ), u_gridSize - vec3( 1.0 ) ) );

	// 2. Compute rayStepX, rayStepY, rayStepZ
	vec3 rayStep = -dirSignBits + notDirSignBits;

	// TODO: check if it is faster to use logic operations or to branch based on ray.dir signs for each dimension

	// 3 Compute out of grid limits
	vec3 outLimit = -dirSignBits + u_gridSize * notDirSignBits;
	
	// 4. Compute tDeltaX, tDeltaY, tDeltaZ
	vec3 tDelta = abs( u_cellSize * g_rayInvDir );

	// 5. Compute tNextX, tNextY, tNextZ
	vec3 tMax = ( voxelToWorld( cellCoords + notDirSignBits ) - g_rayOrig ) * g_rayInvDir;
	
	/************************************************************************/
	/* Trace ray through grid                                               */
	/************************************************************************/
	// Find first non-empty cell
	vec2 cell = gridCellAt( cellCoords );

	// Minimum tMax in all 3 dimensions, used for logical comparison to determine next cell
	float minTmax;

	// Stores 1 for next cell dimension and 0 for the others, used to select next cell
	vec3 comp;

	// While inside grid
	do
	{
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
			intersectTriangles( cell, minTmax );

			if( g_hit.distance < MAX_VALUE )
				return true;
		}

		// Get next cell
		// The above code could all go here, but it is faster to do it early
		cell = gridCellAt( cellCoords );

	} while( cellCoords.x != outLimit.x && cellCoords.y != outLimit.y && cellCoords.z != outLimit.z );
	
	return false;
}

// Needs pre-computed hit position and normal
vec3 shadePhong()
{
	// Get material properties ( diffuseColor.xyz, textureId )
	vec4 diffuseAttrib = getTexel2DRect( u_texMaterials, g_hit.triangleId );

	// Compute direction towards light
	vec3 directionTowardsLight = normalize( g_lightPosition - g_hitPos );

	// Specular reflection
	vec3 lightSpecular = vec3( 0.0 );

	// Compute specular vector
	vec3 specularVector = normalize( reflect( g_rayDir, g_hitNormal ) );

	// Specular factor
	float specDotL = dot( specularVector, directionTowardsLight );
	if( specDotL > 0.0 )
		lightSpecular = g_lightColor * pow( specDotL, g_specularExp );

	// Diffuse and specular shading
	float nDotL = dot( g_hitNormal, directionTowardsLight );
	vec3 lightDiffuse = g_lightColor * nDotL;

#if OVERRIDE_COLOR
	vec3 diffuse = max( vec3( 0.0 ), vec3( 0.0, 0.0, 0.7 ) * lightDiffuse );
#else
	vec3 diffuse = max( vec3( 0.0 ), diffuseAttrib.xyz * lightDiffuse );
#endif
	vec3 specular = g_specular * lightSpecular;

#if ENABLE_TEXTURE
	// Modulate texture color to diffuse component only
	if( diffuseAttrib.w >= 0.0 )
		diffuse *= getTextureColor( diffuseAttrib.w );
#endif

#if ENABLE_SHADOW
	// Trace shadow ray
	vec3 previousRayDir = g_rayDir;
	g_rayOrig = g_hitPos + g_hitNormal * 1e-3;
	g_rayDir  = directionTowardsLight;

	g_hit.distance = MAX_VALUE;
	bool inShadow = rayTrace();

	if( inShadow )
	{
		diffuse *= g_shadowCoeff;
		specular = vec3( 0.0 );
	}

	g_rayDir = previousRayDir;
#endif

	return g_ambient + diffuse + specular;
}

// Headlight illumination
vec3 shadeHeadlight()
{
	// Get material properties ( diffuseColor.xyz, textureId )
	vec4 diffuseAttrib = getTexel2DRect( u_texMaterials, g_hit.triangleId );

	// Need normalized ray direction
	vec3 rayDirNormalized = normalize( g_rayDir );
	
	float nDotD = -dot( g_hitNormal, rayDirNormalized );
	return diffuseAttrib.xyz * ( g_ambient + ( diffuseAttrib.xyz - g_ambient ) * nDotD );
}

vec3 shadeTest()
{
	// Need normalized ray direction
	vec3 rayDirNormalized = normalize( g_rayDir );

	vec3 ambient = vec3( 0.2, 0.2, 0.2 );
	vec3 diffuse = vec3( 0, 0, 1 );

	// Headlight illumination
	float nDotD = -dot( g_hitNormal, rayDirNormalized );
	vec3 sampleColor = ( ambient + ( diffuse - ambient ) * nDotD ) * diffuse;
	return sampleColor;
}

void setupPrimaryRay()
{
	g_rayOrig = u_rayOrig;
    g_rayDir = v_rayDir;
}

void setupReflectionRay()
{
	g_rayOrig = g_hitPos + g_hitNormal * 1e-3;
	g_rayDir = reflect( g_rayDir, g_hitNormal );
}

bool setupRefractionRay( vec3 rayDir, float ratio )
{
	g_rayOrig = g_hitPos - g_hitNormal * 1e-3;
	vec3 newRayDir = refract( rayDir, g_hitNormal, ratio );
	g_rayDir = newRayDir;

	return g_rayDir.x != 0.0 || g_rayDir.y != 0.0 || g_rayDir.z != 0.0;
}

void main()
{
	///////////////////////////////////////////////////////
	// Primary ray
	///////////////////////////////////////////////////////
	setupPrimaryRay();
	bool foundHit = rayTrace();

	if( !foundHit )
	{
		gl_FragColor.rgb = g_background;
		return;
	}

	// Save primary ray dir to trace secondary rays
	//vec3 previousRayDir0 = g_rayDir;

#if ENABLE_SHADING
	// Perform shading
	updateHitPositionAndNormal();
	gl_FragColor.rgb = g_primaryCoeff * shadePhong();


	///////////////////////////////////////////////////////
	// Secondary rays
	///////////////////////////////////////////////////////
#if ENABLE_REFLECTION
	setupReflectionRay();
	foundHit = rayTrace();

	if( foundHit )
	{
		updateHitPositionAndNormal();
		gl_FragColor.rgb += g_reflexCoeff * shadePhong();

//		setupReflectionRay();
//		foundHit = rayTrace();
//
//		if( foundHit )
//		{
//			updateHitPositionAndNormal();
//			gl_FragColor.rgb += g_reflexCoeff * g_reflexCoeff * shadePhong();
//		}
//#if REFLECT_BACKGROUND
//		else
//		{
//			gl_FragColor.rgb += g_reflexCoeff * g_reflexCoeff * g_background;
//		}
//#endif
	}
#if REFLECT_BACKGROUND
	else
	{
		gl_FragColor.rgb += g_reflexCoeff * g_background;
	}
#endif

#endif

#else

	gl_FragColor.rgb = vec3( 0.2, 0.2, 0.2 );

#endif

}

//
//#if ENABLE_REFLECTION
//	setupReflectionRay();
//	foundHit = rayTrace();
//
//#if REFLECT_BACKGROUND
//	if( !foundHit )
//	{
//		gl_FragColor.rgb += g_reflexCoeff * g_background;
//	}
//	else
//#endif
//	{
//		vec3 previousRayDir1 = g_rayDir;
//
//		updateHitPositionAndNormal();
//		gl_FragColor.rgb += g_reflexCoeff * shadePhong();
//
//		setupReflectionRay();
//		foundHit = rayTrace();
//
//#if REFLECT_BACKGROUND
//		if( !foundHit )
//		{
//			gl_FragColor.rgb += g_reflexCoeff * g_reflexCoeff * g_background;
//		}
//		else
//#endif
//		{
//			updateHitPositionAndNormal();
//			gl_FragColor.rgb += g_reflexCoeff * g_reflexCoeff * shadePhong();
//
//#if ENABLE_ALL_REFLECTIONS
//			setupReflectionRay();
//			foundHit = rayTrace();
//
//#if REFLECT_BACKGROUND
//			if( !foundHit )
//			{
//				gl_FragColor.rgb += g_reflexCoeff * g_reflexCoeff * g_reflexCoeff * g_background;
//			}
//			else
//#endif
//			{
//				updateHitPositionAndNormal();
//				gl_FragColor.rgb += g_reflexCoeff * g_reflexCoeff * g_reflexCoeff * shadePhong();
//
//				setupReflectionRay();
//				foundHit = rayTrace();
//
//				if( !foundHit )
//				{
//					gl_FragColor.rgb += g_reflexCoeff * g_reflexCoeff * g_reflexCoeff * g_reflexCoeff * g_background;
//				}
//				else
//				{
//					updateHitPositionAndNormal();
//					gl_FragColor.rgb += g_reflexCoeff * g_reflexCoeff * g_reflexCoeff * g_reflexCoeff * shadePhong();
//				}
//			}
//#endif
//		}
//#if ENABLE_REFRACTION
//		// Entering
//		bool haveRefraction = setupRefractionRay( previousRayDir1, 1.0 / g_refractIndex );
//		if( haveRefraction )
//		{
//			foundHit = rayTrace();
//
//			if( !foundHit )
//			{
//				gl_FragColor.rgb += g_reflexCoeff * g_refractCoeff * g_background;
//			}
//			else
//			{
//				updateHitPositionAndNormal();
//				gl_FragColor.rgb += g_reflexCoeff * g_refractCoeff * shadePhong();
//			}
//		}
//#endif
//	}
//#endif
//
//#if ENABLE_REFRACTION
//	// Entering
//	bool haveRefraction = setupRefractionRay( previousRayDir0, 1.0 / g_refractIndex );
//	if( haveRefraction )
//	{
//		foundHit = rayTrace();
//
//		if( !foundHit )
//		{
//			gl_FragColor.rgb += g_refractCoeff * g_background;
//		}
//		else
//		{
//			vec3 previousRayDir1 = g_rayDir;
//
//			updateHitPositionAndNormal();
//			gl_FragColor.rgb += g_refractCoeff * shadePhong();
//
//			setupReflectionRay();
//			foundHit = rayTrace();
//
//			if( !foundHit )
//			{
//				gl_FragColor.rgb += g_refractCoeff * g_reflexCoeff * g_background;
//			}
//			else
//			{
//				vec3 previousRayDir2 = g_rayDir;
//
//				updateHitPositionAndNormal();
//				gl_FragColor.rgb += g_refractCoeff * g_reflexCoeff * shadePhong();
//
//				setupReflectionRay();
//				foundHit = rayTrace();
//
//				if( !foundHit )
//				{
//					gl_FragColor.rgb += g_refractCoeff * g_reflexCoeff * g_reflexCoeff * g_background;
//				}
//				else
//				{
//					updateHitPositionAndNormal();
//					gl_FragColor.rgb += g_refractCoeff * g_reflexCoeff * g_reflexCoeff * shadePhong();
//				}
//
//				// If exiting
//				if( dot( previousRayDir2, g_hitNormal ) > 0.0 )
//				{
//					// Exiting, need to invert surface normal
//					g_hitNormal = -g_hitNormal;
//					bool haveRefraction = setupRefractionRay( previousRayDir2, g_refractIndex / 1.0 );
//					if( haveRefraction )
//					{
//						foundHit = rayTrace();
//
//						if( !foundHit )
//						{
//							gl_FragColor.rgb += g_refractCoeff * g_reflexCoeff * g_refractCoeff * g_background;
//						}
//						else
//						{
//							updateHitPositionAndNormal();
//							gl_FragColor.rgb += g_refractCoeff * g_reflexCoeff * g_refractCoeff * shadePhong();
//						}
//					}
//				}
//			}
//
//			// If exiting
//			if( dot( previousRayDir1, g_hitNormal ) > 0.0 )
//			{
//				// Exiting, need to invert surface normal
//				g_hitNormal = -g_hitNormal;
//				bool haveRefraction = setupRefractionRay( previousRayDir1, g_refractIndex / 1.0 );
//				if( haveRefraction )
//				{
//					foundHit = rayTrace();
//
//					if( !foundHit )
//					{
//						gl_FragColor.rgb += g_refractCoeff * g_refractCoeff * g_background;
//					}
//					else
//					{
//						vec3 previousRayDir2 = g_rayDir;
//
//						updateHitPositionAndNormal();
//						gl_FragColor.rgb += g_refractCoeff * g_refractCoeff * shadePhong();
//
//						// Entering
//						bool haveRefraction = setupRefractionRay( previousRayDir2, 1.0 / g_refractIndex );
//						if( haveRefraction )
//						{
//							foundHit = rayTrace();
//
//							if( !foundHit )
//							{
//								gl_FragColor.rgb += g_refractCoeff * g_refractCoeff * g_refractCoeff * g_background;
//							}
//							else
//							{
//								vec3 previousRayDir3 = g_rayDir;
//
//								updateHitPositionAndNormal();
//								gl_FragColor.rgb += g_refractCoeff * g_refractCoeff * g_refractCoeff * shadePhong();
//
//								// If exiting
//								if( dot( previousRayDir3, g_hitNormal ) > 0.0 )
//								{
//									// Exiting, need to invert surface normal
//									g_hitNormal = -g_hitNormal;
//									bool haveRefraction = setupRefractionRay( previousRayDir3, g_refractIndex / 1.0 );
//									if( haveRefraction )
//									{
//										foundHit = rayTrace();
//
//										if( !foundHit )
//										{
//											gl_FragColor.rgb += g_refractCoeff * g_refractCoeff * g_refractCoeff * g_refractCoeff * g_background;
//										}
//										else
//										{
//											updateHitPositionAndNormal();
//											gl_FragColor.rgb += g_refractCoeff * g_refractCoeff * g_refractCoeff * g_refractCoeff * shadePhong();
//										}
//									}
//								}
//							}
//						}
//					}
//				}
//			}
//		}
//	}
//#endif
//
//




//
//
//
//
//
//
//
//
//
//// 3D-DDA ray-grid traversal (Amanatides & Woo)
//void main()
//{
//	g_rayOrig = u_rayOrig;
//
//	// Trace primary ray
//	bool haveHit = rayTrace();
//
//	// Background color
//	vec3 sampleColor = g_background;
//
//	if( haveHit )
//	{
//		// Compute hit position and normal
//		updateHitPositionAndNormal();
//
//		// Save previous raydir
//		vec3 primaryRayDir = g_rayDir;
//
//		// Perform shading computations
//		sampleColor = shadePhong();
//
//#if ENABLE_REFLECTION
//		// Setup reflection ray
//		g_rayOrig = g_hitPos + g_hitNormal * 1e-3;
//		g_rayDir = reflect( primaryRayDir, g_hitNormal );
//
//		g_hit.distance = MAX_VALUE;
//		bool haveReflection = rayTrace();
//
//		if( haveReflection )
//		{
//			// Compute hit position and normal
//			updateHitPositionAndNormal();
//
//			// Perform shading computations
//			sampleColor += g_reflexCoeff * shadePhong();
//
//			// Secondary reflection
//#if ENABLE_SECONDARY_REFLECTION
//			// Setup reflection ray
//			g_rayOrig = g_hitPos + g_hitNormal * 1e-3;
//			g_rayDir = reflect( primaryRayDir, g_hitNormal );
//
//			g_hit.distance = MAX_VALUE;
//			haveReflection = rayTrace();
//
//			if( haveReflection )
//			{
//				// Compute hit position and normal
//				updateHitPositionAndNormal();
//				sampleColor += g_reflexCoeff * g_reflexCoeff * shadePhong();
//			}
//			else
//			{
//				//sampleColor += g_reflexCoeff * g_reflexCoeff * g_background;
//			}
//#endif
//		}
//		else
//		{
//			//sampleColor += g_reflexCoeff * g_background;
//		}
//#endif
//
//#if ENABLE_REFRACTION
//		primaryRayDir = normalize( primaryRayDir );
//
//		// Setup refraction ray
//		g_rayOrig = g_hitPos - g_hitNormal * 1e-3;
//		vec3 newRayDir = refract( primaryRayDir, g_hitNormal, 1.0 / 1.2 );
//
//		if( newRayDir.x != 0.0 || newRayDir.y != 0.0 || newRayDir.z != 0.0 )
//		{
//			g_rayDir = newRayDir;
//			g_hit.distance = MAX_VALUE;
//			bool haveRefraction = rayTrace();
//
//			if( haveRefraction )
//			{
//				// Compute hit position and normal
//				updateHitPositionAndNormal();
//
//				// Perform shading computations
//				sampleColor += g_refracCoeff * shadePhong();
//
//				// If entered an object, need to exit
//				g_hitNormal = -g_hitNormal;
//
//				// Setup refraction ray
//				g_rayOrig = g_hitPos - g_hitNormal * 1e-3;
//				newRayDir = refract( newRayDir, g_hitNormal, 1.2 / 1.0 );
//
//				if( newRayDir.x != 0.0 || newRayDir.y != 0.0 || newRayDir.z != 0.0 )
//				{
//					g_rayDir = newRayDir;
//					g_hit.distance = MAX_VALUE;
//					bool haveRefraction = rayTrace();
//
//					if( haveRefraction )
//					{
//						// Compute hit position and normal
//						updateHitPositionAndNormal();
//
//						// Perform shading computations
//						sampleColor += g_refracCoeff * g_refracCoeff * shadePhong();
//					}
//					else
//					{
//						sampleColor += g_refracCoeff * g_refracCoeff * g_background;
//					}
//				}
//			}
//			else
//			{
//				sampleColor += g_refracCoeff * g_background;
//			}
//		}
//#endif
//	}
//
//	gl_FragColor.rgb = sampleColor;
//}
