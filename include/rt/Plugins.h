#ifndef _RT_PLUGINS_H_
#define _RT_PLUGINS_H_

#include <rt/common.h>
#include <rt/IAccStructBuilder.h>
#include <rt/ICamera.h>
#include <rt/IEnvironment.h>
#include <rt/IRenderer.h>
#include <rt/ILight.h>
#include <rt/IMaterial.h>
#include <rt/ITexture.h>

namespace rt {

struct Plugins
{
	vr::ref_ptr<rt::IAccStructBuilder> accStructBuilder;
	vr::ref_ptr<rt::ICamera> camera;
	vr::ref_ptr<rt::IEnvironment> environment;
	vr::ref_ptr<rt::IRenderer> renderer;

	std::vector< vr::ref_ptr<rt::ILight> > lights;
	std::vector< vr::ref_ptr<rt::IMaterial> > materials;
	std::vector< vr::ref_ptr<rt::ITexture> > textures;
};

} // namespace rt

#endif // _RT_PLUGINS_H_
