#ifndef _RT_INSTANCE_H_
#define _RT_INSTANCE_H_

#include <rt/common.h>
#include <rt/Transform.h>
#include <rt/Aabb.h>

namespace rt {

// Forward declaration
class Geometry;

struct Instance
{
	Instance();

	Geometry* geometry;
	Transform transform;
	Aabb      bbox;
};

} // namespace rt

#endif // _RT_INSTANCE_H_
