#include <rt/IAccStruct.h>
#include <rt/Context.h>
#include <rt/Ray.h>
#include <rt/Hit.h>
#include <rt/AabbIntersection.h>
#include <rt/RayTriIntersection.h>
#include <rt/IEnvironment.h>

using namespace rt;

void IAccStruct::clear()
{
	// empty
}

void IAccStruct::setBoundingBox( const rt::Aabb& bbox )
{
	_bbox = bbox;
}

const rt::Aabb& IAccStruct::getBoundingBox() const
{
	return _bbox;
}

void IAccStruct::traceNearestInstance( const std::vector<rt::Instance>& instances, rt::Sample& sample )
{
	rt::Ray& ray = sample.ray;
	rt::Hit& hit = sample.hit;

	rt::Context* ctx = rt::Context::current();

	// Init ray
	ray.tnear = ctx->getRayEpsilon();
	ray.tfar = vr::Mathf::MAX_VALUE;
	ray.update();

	// Init hit
	hit.instance = NULL;
	hit.distance = vr::Mathf::MAX_VALUE;

	// If not hit bbox of entire scene, no need to trace any further
	if( !rt::AabbIntersection::clipRay( _bbox, ray ) )
	{
		ctx->getEnvironment()->shade( sample );
		return;
	}

	// Save original ray to restore after instance matrix transformations
	const rt::Ray originalRay( ray );

	// Trace instances
	for( uint32 i = 0, limit = instances.size(); i < limit; ++i )
	{
		const rt::Instance& instance = instances[i];

		// Transform ray to geometry's local space
		instance.transform.inverseTransform( ray );
		ray.update();

		// Ask geometry's acceleration structure to trace the transformed ray
		instance.geometry->accStruct->traceNearestGeometry( instance, ray, hit );

		// Transform ray back to global space
		ray = originalRay;
	}

	if( hit.instance )
		hit.instance->geometry->triDesc[hit.triangleId].material->shade( sample );
	else
		ctx->getEnvironment()->shade( sample );
}

void IAccStruct::traceNearestGeometry( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit )
{
	// If don't hit bbox in local space, no need to trace underlying triangles
	if( !rt::AabbIntersection::clipRay( _bbox, ray ) )
		return;

	const rt::Geometry& geometry = *instance.geometry;

	// Best distance considers any previously found hits to prevent false hits in this geometry
	float bestDistance = hit.distance;

	for( uint32 t = 0, limit = geometry.triAccel.size(); t < limit; ++t )
	{
		rt::RayTriIntersection::hitWald( geometry.triAccel[t], ray, hit, bestDistance );
		//rt::RayTriIntersection::hitMT1( geometry, t, ray, hit, bestDistance );
		//rt::RayTriIntersection::hitMT2( geometry, t, ray, hit, bestDistance );
		//rt::RayTriIntersection::hitMT3( geometry, t, ray, hit, bestDistance );
		//rt::RayTriIntersection::hitMT4( geometry, t, ray, hit, bestDistance );
		//rt::RayTriIntersection::hitMT5( geometry, t, ray, hit, bestDistance );
		//rt::RayTriIntersection::hitMT6( geometry, t, ray, hit, bestDistance );
		//rt::RayTriIntersection::hitMT7( geometry, t, ray, hit, bestDistance );
		//rt::RayTriIntersection::hitTest( geometry, t, ray, hit, bestDistance );
		//rt::RayTriIntersection::hitChirkov( geometry, t, ray, hit, bestDistance );
		//rt::RayTriIntersection::hitHalfSpace( geometry, t, ray, hit, bestDistance );
		//rt::RayTriIntersection::hitSignedVolume( geometry, t, ray, hit, bestDistance );
	}

	if( bestDistance < hit.distance )
	{
		hit.distance = bestDistance;
		hit.instance = &instance;
	}
}

bool IAccStruct::traceAnyInstance( const std::vector<rt::Instance>& instances, rt::Sample& sample )
{
	// TODO: 
	// avoid warnings
	sample;
	return false;
}

bool IAccStruct::traceAnyGeometry( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit )
{
	// TODO: 
	// avoid warnings
	instance;ray;hit;
	return false;
}
