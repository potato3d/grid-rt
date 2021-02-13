#ifndef _RTP_SUPERSAMPLEADAPTIVERENDERER_H_
#define _RTP_SUPERSAMPLEADAPTIVERENDERER_H_

#include <rt/IRenderer.h>
#include <vr/vec3.h>

namespace rtp {

class SuperSampleAdaptiveRenderer : public rt::IRenderer
{
public:
	SuperSampleAdaptiveRenderer();

	virtual void render();

private:
	void adaptiveSupersample( float x, float y, vr::vec3f& resultColor, uint32 recursionDepth );

	uint32 _maxRecursionDepth;
	float _epsilon;
};

} // namespace rtp

#endif // _RTP_SUPERSAMPLEADAPTIVERENDERER_H_
