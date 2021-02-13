#ifndef _RTP_MULTITHREADRENDERER_H_
#define _RTP_MULTITHREADRENDERER_H_

#include <rt/IRenderer.h>

namespace rtp {

class MultiThreadRenderer : public rt::IRenderer
{
public:
	virtual void render();
};

} // namespace rtp

#endif // _RTP_MULTITHREADRENDERER_H_
