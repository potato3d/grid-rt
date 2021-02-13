#ifndef _RTP_TILEDRENDERER_H_
#define _RTP_TILEDRENDERER_H_

#include <rt/IRenderer.h>

namespace rtp {

class TiledRenderer : public rt::IRenderer
{
public:
	virtual void render();
};

} // namespace rtp

#endif // _RTP_TILEDRENDERER_H_
