#version 120
#extension GL_ARB_texture_rectangle : enable
#extension GL_EXT_gpu_shader4 : enable

// Reflection
uniform bool u_reflectionPass;

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
const vec3 g_lightPosition = vec3( 1e10, 1e10, 1e10 );
const vec3 g_lightColor = vec3( 1.0, 1.0, 1.0 );

// Texture access information
uniform sampler2DRect  u_texMaterials;
uniform sampler2DArray u_texMaterialTextureArray;

// Scene data
uniform sampler2DRect u_texTriangleNormals;
uniform sampler2DRect u_texTriangleTexcoords;

// Ray data
uniform vec3 u_rayOrig;
varying vec3 v_rayDir;

// Hit data
uniform sampler2DRect u_texHits;
vec4 g_hit;

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

vec3 getTextureColor( float textureId )
{
	// Get triangle texcoords
	// triangle id's are sequential, but we have 3 texcoords per triangle
	// texture is luminance_alpha
    vec3 t0 = getTexel2DRect( u_texTriangleTexcoords, g_hit.x * 3.0 ).raa;
    vec3 t1 = getTexel2DRect( u_texTriangleTexcoords, g_hit.x * 3.0 + 1.0 ).raa;
    vec3 t2 = getTexel2DRect( u_texTriangleTexcoords, g_hit.x * 3.0 + 2.0 ).raa;

	// TODO: for hand model
	//t0.y = 1.0 - t0.y;
	//t1.y = 1.0 - t1.y;
	//t2.y = 1.0 - t2.y;
	
	// TODO: for ben model
	//t0.x = 1.0 - t0.x;
	//t1.x = 1.0 - t1.x;
	//t2.x = 1.0 - t2.x;
	
	vec3 t = t0 * ( 1.0 - ( g_hit.y + g_hit.z ) ) +	// v0 coord
		     t1 * g_hit.y +						// v1 coord
			 t2 * g_hit.z;						// v2 coord

	// Use texture index to access texture array
	t.z = textureId;

	return texture2DArray( u_texMaterialTextureArray, t ).rgb;
}

void main()
{
	// Recover hit information
	g_hit = texture2DRect( u_texHits, gl_FragCoord.xy );

	// If no hit, background color
	if( g_hit.x == MAX_VALUE )
	{
		gl_FragColor = u_reflectionPass? vec4( 0.0 ) : vec4( 1.0 );
		return;
	}

	bool inShadow = g_hit.x < 0.0;
	g_hit.x = abs( g_hit.x );

	// Compute hit position
	vec3 hitPos = u_rayOrig + v_rayDir * g_hit.w;

	// Compute shading normal
	vec3 hitNormal = computeShadingNormal();

	// Get material properties ( diffuseColor.xyz, textureId )
	vec4 diffuseAttrib = getTexel2DRect( u_texMaterials, g_hit.x );

	// Compute direction towards light
	vec3 directionTowardsLight = normalize( g_lightPosition - hitPos );

	// Specular reflection
	vec3 lightSpecular = vec3( 0.0 );

	// Compute specular vector
	vec3 specularVector = normalize( reflect( v_rayDir, hitNormal ) );

	// Specular factor
	float specDotL = dot( specularVector, directionTowardsLight );
	if( specDotL > 0.0 )
		lightSpecular = g_lightColor * pow( specDotL, g_specularExp );

	// Diffuse and specular shading
	float nDotL = dot( hitNormal, directionTowardsLight );
	vec3 lightDiffuse = g_lightColor * nDotL;

	vec3 diffuse = max( vec3( 0.0 ), diffuseAttrib.xyz * lightDiffuse );

	vec3 specular = g_specular * lightSpecular;
	
	// Modulate texture color to diffuse component only
	if( diffuseAttrib.w >= 0.0 )
		diffuse *= getTextureColor( diffuseAttrib.w );

	// Dimm diffuse component if in shadow
	if( inShadow )
		gl_FragColor.rgb = g_ambient + diffuse * g_shadowCoeff;
	else
		gl_FragColor.rgb = g_ambient + diffuse + specular;

	if( u_reflectionPass )
		gl_FragColor *= g_reflexCoeff;
}