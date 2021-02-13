#include <rt/Sample.h>
#include <rt/Context.h>
#include <rt/ICamera.h>

using namespace rt;

void Sample::reflectDirectionVector( const vr::vec3f& incident, const vr::vec3f& normal, vr::vec3f& reflected )
{
	const float proj = incident.dot( normal ) * 2.0f;
	reflected = incident - ( normal * proj );
}

void Sample::initPrimaryRay( float x, float y )
{
	rt::Context::current()->getCamera()->getRayOrigin( ray.orig, x, y );
	rt::Context::current()->getCamera()->getRayDirection( ray.dir, x, y );

	// Reset ray state parameters
	recursionDepth = 0;
}

void Sample::initLightRay( const Sample& base )
{
	ray.dir = base.ray.dir;
	// TODO: shoudn't need an epsilon here...
	hitPosition = base.hitPosition + base.normal * rt::Context::current()->getRayEpsilon();
	normal = base.normal;
}

bool Sample::initShadowRay( const vr::vec3f& directionTowardsLight, float rayMaxDistance )
{
	// Check if surface does not point away from light
	const float nDotL = normal.dot( directionTowardsLight );
	if( nDotL <= 0.0f )
		return false;

	// Set new ray parameters
	ray.orig = hitPosition;
	ray.dir = directionTowardsLight;
	ray.tfar = rayMaxDistance;
	return true;
}

void Sample::initReflectionRay( const Sample& base )
{
	// Set new ray origin and direction
	ray.orig = base.hitPosition;
	reflectDirectionVector( base.ray.dir, base.normal, ray.dir );

	// Increment recursion depth
	recursionDepth = base.recursionDepth + 1;
}

bool Sample::initRefractionRay( const Sample& base, float refractionIndex )
{
	// First, we need to determine if we are entering or exiting the surface we hit in order to get the correct 
	// normal orientation and ratio of refraction indexes.
	vr::vec3f normal;
	float n;

	// If front face
	if( base.isFrontFace() )
	{
		// Entering the object
		normal = base.normal;
		n = rt::Context::current()->getMediumRefractionIndex() / refractionIndex;
	}
	else
	{
		// Exiting the object
		normal = -base.normal;
		n = refractionIndex / rt::Context::current()->getMediumRefractionIndex();
	}

	// Get cosine of incident angle: need normalized ray direction
	vr::vec3f normalizedRayDir = base.ray.dir;
	normalizedRayDir.normalize();
	const float cosI = -( normal.dot( normalizedRayDir ) );

	// Now compute transmitted angle and check for total internal reflection
	const float cosTsquare = 1.0f - n * n * ( 1.0f - cosI * cosI );
	if( cosTsquare <= 0.0f )
		return false;

	// Finally, compute transmitted ray direction and the other easy stuff
	const float cosT = sqrtf( cosTsquare );

	ray.dir = normalizedRayDir * n + normal * ( n * cosI - cosT );

	ray.orig = base.hitPosition;
	recursionDepth = base.recursionDepth + 1;
	return true;
}

void Sample::computeHitPosition()
{
	hitPosition = ray.orig + ray.dir * hit.distance;
}

void Sample::computeShadingNormal()
{
	const Geometry& geometry = *hit.instance->geometry;

	const vr::vec3f& v0Normal = geometry.getNormal( hit.triangleId, 0 );
	const vr::vec3f& v1Normal = geometry.getNormal( hit.triangleId, 1 );
	const vr::vec3f& v2Normal = geometry.getNormal( hit.triangleId, 2 );

	normal = v0Normal * hit.v0Coord + 
		     v1Normal * hit.v1Coord + 
			 v2Normal * hit.v2Coord;

	hit.instance->transform.transformNormal( normal );
	normal.normalize();
}

void Sample::computeShadingColor()
{
	const Geometry& geometry = *hit.instance->geometry;

	const vr::vec3f& v0Color = geometry.getColor( hit.triangleId, 0 );
	const vr::vec3f& v1Color = geometry.getColor( hit.triangleId, 1 );
	const vr::vec3f& v2Color = geometry.getColor( hit.triangleId, 2 );

	color = v0Color * hit.v0Coord + 
		    v1Color * hit.v1Coord + 
			v2Color * hit.v2Coord;
}

void Sample::computeTexCoords( vr::vec3f& texCoords )
{
	const Geometry& geometry = *hit.instance->geometry;

	const vr::vec3f& v0TexCoord = geometry.getTexCoords( hit.triangleId, 0 );
	const vr::vec3f& v1TexCoord = geometry.getTexCoords( hit.triangleId, 1 );
	const vr::vec3f& v2TexCoord = geometry.getTexCoords( hit.triangleId, 2 );

	texCoords = v0TexCoord * hit.v0Coord + 
		        v1TexCoord * hit.v1Coord + 
				v2TexCoord * hit.v2Coord;
}

void Sample::computeFlatNormal()
{
	const Geometry& geometry = *hit.instance->geometry;

	const vr::vec3f& v0 = geometry.getVertex( hit.triangleId, 0 );
	const vr::vec3f& v1 = geometry.getVertex( hit.triangleId, 1 );
	const vr::vec3f& v2 = geometry.getVertex( hit.triangleId, 2 );

	normal = ( v1 - v0 ).cross( v2 - v0 );

	hit.instance->transform.transformNormal( normal );
	normal.normalize();
}

bool Sample::isFrontFace() const
{
	return ( normal.dot( ray.dir ) <= 0.0f );
}

void Sample::computeSpecularVector( vr::vec3f& v )
{
	reflectDirectionVector( ray.dir, normal, v );
	v.normalize();
}

bool Sample::stopRayRecursion() const
{
	return recursionDepth >= rt::Context::current()->getMaxRecursionDepth();
}
