#ifndef _RT_IRENDERER_H_
#define _RT_IRENDERER_H_

#include <rt/IPlugin.h>

namespace rt {

class IRenderer : public IPlugin
{
public:
	// Default implementation: do nothing
	virtual void render();
};

} // namespace rt

#endif // _RT_IRENDERER_H_
