#include <rtp/SingleThreadRenderer.h>
#include <rt/Context.h>
#include <rt/ICamera.h>
#include <rt/IEnvironment.h>

using namespace rtp;

void SingleThreadRenderer::render()
{
	uint32 width;
	uint32 height;
	rt::Context* ctx = rt::Context::current();
	ctx->getCamera()->getViewport( width, height );
	float* frameBuffer = ctx->getFrameBuffer();
	rt::Sample sample;
	uint32 pixel = 0;

	for( uint32 y = 0; y < height; ++y )
	{
		for( uint32 x = 0; x < width; ++x )
		{
			sample.initPrimaryRay( x, y );
			ctx->traceNearest( sample );

			frameBuffer[pixel] = sample.color.r;
			++pixel;
			frameBuffer[pixel] = sample.color.g;
			++pixel;
			frameBuffer[pixel] = sample.color.b;
			++pixel;
		}
	}
}
