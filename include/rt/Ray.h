#ifndef _RT_RAY_H_
#define _RT_RAY_H_

#include <rt/common.h>
#include <vr/vec3.h>

namespace rt {

struct Ray
{
	// Pre-compute invDir and dirSigns, used in ray-box intersection and hierarchy traversal.
	// Must be called whenever the ray direction changes (i.e. ray is transformed)
	inline void update();

	vr::vec3f orig;
	vr::vec3f dir;
	float tnear;
	float tfar;

	// Pre-computations
	vr::vec3f invDir;
	int32 dirSigns[3]; // neg = -1, pos = 1
	int32 dirSignBits[3]; // neg = 1, pos = 0
};

inline void Ray::update()
{
	invDir.x = 1.0f / dir.x;
	invDir.y = 1.0f / dir.y;
	invDir.z = 1.0f / dir.z;
	dirSigns[0] = (int32)vr::sign( dir.x );
	dirSigns[1] = (int32)vr::sign( dir.y );
	dirSigns[2] = (int32)vr::sign( dir.z );
	dirSignBits[0] = (int32)vr::signBit( dir.x );
	dirSignBits[1] = (int32)vr::signBit( dir.y );
	dirSignBits[2] = (int32)vr::signBit( dir.z );
}

} // namespace rt

#endif // RT_RAY_H
