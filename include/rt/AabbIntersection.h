#ifndef _RT_AABBINTERSECTION_H_
#define _RT_AABBINTERSECTION_H_

#include <rt/common.h>
#include <rt/Aabb.h>
#include <rt/SplitPlane.h>
#include <rt/Ray.h>

namespace rt {

class AabbIntersection
{
public:
	// Returns false if ray segment is completely outside box. Returns true otherwise.
	// Only update ray if found valid clipping (return true).
	static bool clipRay( const Aabb& box, Ray& ray );

	static void clipTriangle( const vr::vec3f& v0, const vr::vec3f& v1, const vr::vec3f& v2, 
		                      const Aabb& box, Aabb& result );

	static void splitAabb( const Aabb& box, const SplitPlane& plane, Aabb& left, Aabb& right );

	static bool triangleOverlaps( const Aabb& box, const vr::vec3f& v0, const vr::vec3f& v1, const vr::vec3f& v2 );

	static bool isPointInside( const Aabb& box, const vr::vec3f& point );

private:
	static void clip( vr::vec3f vertexBuffer[8], vr::vec3f tempBuffer[8], 
		              uint32& vertexCount, float pos, float dir, RTenum dim );

	// Triangle-ABB Overlap Axis Tests
	// These were defines in original algorithm
	static void axisTestX( const vr::vec3f& boxHalfSizes, const vr::vec3f& vA, const vr::vec3f& vB, 
		                   float a, float b, float fa, float fb, float& minp, float& maxp, float& rad );
	
	static void axisTestY( const vr::vec3f& boxHalfSizes, const vr::vec3f& vA, const vr::vec3f& vB, 
		                   float a, float b, float fa, float fb, float& minp, float& maxp, float& rad );

	static void axisTestZ( const vr::vec3f& boxHalfSizes, const vr::vec3f& vA, const vr::vec3f& vB, 
		                   float a, float b, float fa, float fb, float& minp, float& maxp, float& rad );

	static void findMinMax( float a, float b, float c, float& minp, float& maxp );

	static bool planeBoxOverlap( const vr::vec3f& boxHalfSizes, const vr::vec3f& normal, const vr::vec3f& vertex );
};

} // namespace rt

#endif // _RT_AABBINTERSECTION_H_
