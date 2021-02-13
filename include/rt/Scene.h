#ifndef _RT_SCENE_H_
#define _RT_SCENE_H_

#include <rt/common.h>
#include <rt/Instance.h>
#include <rt/IAccStruct.h>

namespace rt {

struct Scene
{
	vr::ref_ptr<rt::IAccStruct> accStruct;
	std::vector<Instance> instances;
	std::vector< vr::ref_ptr<Geometry> > geometries;
};

} // namespace rt

#endif // _RT_SCENE_H_
