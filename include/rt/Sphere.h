#ifndef _RT_SPHERE_H_
#define _RT_SPHERE_H_

#include <rt/common.h>
#include <vr/vec3.h>

namespace rt {

struct Sphere 
{
	// Based on the algorithm at http://realtimecollisiondetection.net/blog/?p=20
	bool computeFrom( const vr::vec3f& a, const vr::vec3f& b, const vr::vec3f& c );

	vr::vec3f center;
	float radius;
};

} // namespace rt

#endif // _RT_SPHERE_H_
