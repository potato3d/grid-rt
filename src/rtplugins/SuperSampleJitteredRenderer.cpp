#include <rtp/SuperSampleJitteredRenderer.h>
#include <rt/Context.h>
#include <rt/ICamera.h>
#include <vr/random.h>
#include <omp.h>

using namespace rtp;

static const float TWO_BY_TWO_GRID[] = { -0.25f, -0.25f, // line 0
                                          0.25f, -0.25f, 
								         -0.25f,  0.25f, // line 1
								          0.25f,  0.25f };

static const float FOUR_BY_FOUR_GRID[] = { -0.375f, -0.375f, // line 0 
									       -0.125f, -0.375f, 
									        0.125f, -0.375f, 
									        0.375f, -0.375f,
									       -0.375f, -0.125f, // line 1
									       -0.125f, -0.125f,
									        0.125f, -0.125f,
									        0.375f, -0.125f,
									       -0.375f,  0.125f, // line 2
									       -0.125f,  0.125f,
									        0.125f,  0.125f,
									        0.375f,  0.125f,
									       -0.375f,  0.375f, // line 3
								           -0.125f,  0.375f,
									        0.125f,  0.375f,
									        0.375f,  0.375f };

SuperSampleJitteredRenderer::SuperSampleJitteredRenderer()
{
	_gridRes = FOUR_BY_FOUR;
}

void SuperSampleJitteredRenderer::render()
{
	uint32 width;
	uint32 height;
	rt::Context* ctx = rt::Context::current();
	ctx->getCamera()->getViewport( width, height );
	float* frameBuffer = ctx->getFrameBuffer();
	rt::Sample sample;
	int32 w = (int32)width;
	int32 h = (int32)height;
	vr::vec3f resultColor;
	int32 x, y;

	const float* grid;
	uint32 gridSize;
	float ratio;

	switch( _gridRes )
	{
	case TWO_BY_TWO:
		grid = TWO_BY_TWO_GRID;
		gridSize = 8;
		ratio = 0.25f;
		break;

	case FOUR_BY_FOUR:
		grid = FOUR_BY_FOUR_GRID;
		gridSize = 32;
		ratio = 0.0625f;
		break;

	default:
	    return;
	}

	int32 chunk = 16;

	#pragma omp parallel for shared( frameBuffer, h, w ) private( y ) schedule( dynamic, chunk )
	for( y = 0; y < h; ++y )
	{
		#pragma omp parallel for shared( frameBuffer, h, w, y, grid, gridSize, ratio ) private( x, resultColor, sample ) schedule( dynamic, chunk )
		for( x = 0; x < w; ++x )
		{
			resultColor.set( 0.0f, 0.0f, 0.0f );

			uint32 i = 0;
			while( i < gridSize )
			{

				sample.initPrimaryRay( (float)x + grid[i++] + vr::Random::real( -ratio, ratio ), 
					                     (float)y + grid[i++] + vr::Random::real( -ratio, ratio ) );
				ctx->traceNearest( sample );

				resultColor += sample.color;
			}
			
			resultColor *= ratio;
			
			frameBuffer[(x+y*w)*3]   = resultColor.r;
			frameBuffer[(x+y*w)*3+1] = resultColor.g;
			frameBuffer[(x+y*w)*3+2] = resultColor.b;
		}
	}
}

void SuperSampleJitteredRenderer::setGridResolution( GridResolution res )
{
	_gridRes = res;
}
