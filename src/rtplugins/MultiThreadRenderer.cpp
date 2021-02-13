#include <rtp/MultiThreadRenderer.h>
#include <rt/Context.h>
#include <rt/ICamera.h>
#include <omp.h>

using namespace rtp;

void MultiThreadRenderer::render()
{
	uint32 width;
	uint32 height;
	rt::Context* ctx = rt::Context::current();
	ctx->getCamera()->getViewport( width, height );
	float* frameBuffer = ctx->getFrameBuffer();
	rt::Sample sample;
	uint32 pixel = 0;
	int32 x, y;
	int32 w = (int32)width;
	int32 h = (int32)height;

	int32 chunk = 16;

	#pragma omp parallel for shared( ctx, frameBuffer, h, w ) private( y ) schedule( dynamic, chunk )
    for( y = 0; y < h; ++y )
    {
		#pragma omp parallel for shared( ctx, frameBuffer, h, w, y ) private( x, sample ) schedule( dynamic, chunk )
        for( x = 0; x < w; ++x )
        {
			sample.initPrimaryRay( x, y );
			ctx->traceNearest( sample );

            //printf( "--------------\n" );
            //printf( "thread ID: %d\n", omp_get_thread_num() );
            //printf( "pixel: %d\n", pixel );
            //printf( "--------------\n" );
            frameBuffer[(x+y*w)*3]   = sample.color.r;
            frameBuffer[(x+y*w)*3+1] = sample.color.g;
            frameBuffer[(x+y*w)*3+2] = sample.color.b;
        }
    }
}
