#ifndef _RT_HIT_H_
#define _RT_HIT_H_

#include <rt/common.h>
#include <rt/Instance.h>

namespace rt {

struct Hit
{
	uint32 triangleId;
	float v0Coord;
	float v1Coord;
	float v2Coord;
	float distance;
	const Instance* instance;
};

} // namespace rt

#endif // _RT_HIT_H_
