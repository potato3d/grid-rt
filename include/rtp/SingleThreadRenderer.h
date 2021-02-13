#ifndef _RTP_SINGLETHREADRENDERER_H_
#define _RTP_SINGLETHREADRENDERER_H_

#include <rt/IRenderer.h>

namespace rtp {

class SingleThreadRenderer : public rt::IRenderer
{
public:
	virtual void render();
};

} // namespace rtp

#endif // _RTP_SINGLETHREADRENDERER_H_
