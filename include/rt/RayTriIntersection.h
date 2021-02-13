#ifndef _RT_RAYTRIINTERSECTION_H_
#define _RT_RAYTRIINTERSECTION_H_

#include <rt/common.h>
#include <rt/Triangle.h>
#include <rt/Ray.h>
#include <rt/Hit.h>

namespace rt {

class RayTriIntersection
{
public:
	static void hitWald( const rt::TriAccel& acc, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitMT1( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitMT2( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitMT3( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitMT4( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitMT5( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitMT6( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitMT7( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitTest( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitChirkov( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitHalfSpace( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );
	static void hitSignedVolume( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, float& bestDistance );

private:
	static uint32 s_modulo[8];
};

} // namespace rt

#endif // _RT_RAYTRIINTERSECTION_H_
