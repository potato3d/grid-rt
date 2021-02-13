#include <rtp/TiledRenderer.h>
#include <rt/Context.h>
#include <rt/ICamera.h>
#include <omp.h>

using namespace rtp;

void TiledRenderer::render()
{
	uint32 width;
	uint32 height;
	rt::Context* ctx = rt::Context::current();
	ctx->getCamera()->getViewport( width, height );
	float* frameBuffer = ctx->getFrameBuffer();
	rt::Sample sample;
	int32 w = (int32)width;
	int32 h = (int32)height;

	const int32 tileSize = 16;
	const int32 chunk = 1;

	const int32 numTilesX = vr::round( (float)w / (float)tileSize );
	const int32 numTilesY = vr::round( (float)h / (float)tileSize );
	const float invNumTilesX = 1.0f / (float)numTilesX;
	const int32 limit = numTilesX * numTilesY;

	#pragma omp parallel for shared( frameBuffer, tileSize, h, w, numTilesX, ctx ) private( sample ) schedule( dynamic, chunk )
	for( int32 i = 0; i < limit; ++i )
	{
		const int32 ty = ( i / numTilesX ) * tileSize;
		const int32 tx = ( i % numTilesX ) * tileSize;

		for( int32 dy = 0; dy < tileSize; ++dy )
		{
			const int32 y = ty + dy;
			if( y >= h )
				continue;

			for( int32 dx = 0; dx < tileSize; ++dx )
			{
				const int32 x = tx + dx;
				if( x >= w )
					continue;

				sample.initPrimaryRay( x, y );
				ctx->traceNearest( sample );

				frameBuffer[(x+y*w)*3]   = sample.color.r;
				frameBuffer[(x+y*w)*3+1] = sample.color.g;
				frameBuffer[(x+y*w)*3+2] = sample.color.b;
			}
		}
	}
}
