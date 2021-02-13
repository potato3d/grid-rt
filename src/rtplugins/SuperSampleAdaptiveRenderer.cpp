#include <rtp/SuperSampleAdaptiveRenderer.h>
#include <rt/Context.h>
#include <rt/ICamera.h>
#include <vr/random.h>
#include <omp.h>

using namespace rtp;

SuperSampleAdaptiveRenderer::SuperSampleAdaptiveRenderer()
{
	_epsilon = 0.01f;
	_maxRecursionDepth = 3;
}

void SuperSampleAdaptiveRenderer::render()
{
	uint32 width;
	uint32 height;
	rt::Context* ctx = rt::Context::current();
	ctx->getCamera()->getViewport( width, height );
	float* frameBuffer = ctx->getFrameBuffer();
	int32 w = (int32)width;
	int32 h = (int32)height;
	vr::vec3f resultColor;
	int32 x, y;

	int32 chunk = 16;

	#pragma omp parallel for shared( frameBuffer, h, w ) private( y ) schedule( dynamic, chunk )
	for( y = 0; y < h; ++y )
	{
		#pragma omp parallel for shared( frameBuffer, h, w, y ) private( x, resultColor ) schedule( dynamic, chunk )
		for( x = 0; x < w; ++x )
		{
			adaptiveSupersample( x, y, resultColor, 1 );
			
			frameBuffer[(x+y*w)*3]   = resultColor.r;
			frameBuffer[(x+y*w)*3+1] = resultColor.g;
			frameBuffer[(x+y*w)*3+2] = resultColor.b;
		}
	}
}

void SuperSampleAdaptiveRenderer::adaptiveSupersample( float x, float y, vr::vec3f& resultColor, uint32 recursionDepth )
{
	rt::Context* ctx = rt::Context::current();
	const float deltaRatio = 0.5f / (float)recursionDepth;
	float deltaX;
	float deltaY;
	rt::Sample upperLeftSample;
	rt::Sample upperRightSample;
	rt::Sample lowerLeftSample;
	rt::Sample lowerRightSample;

	// Upper left (A)
	deltaX = vr::Random::real( -deltaRatio, 0.0f );
	deltaY = vr::Random::real( 0.0f, deltaRatio );

	upperLeftSample.initPrimaryRay( x + deltaX, y + deltaY );
	ctx->traceNearest( upperLeftSample );
	vr::vec3f upperLeftColor = upperLeftSample.color;

	// Upper right (B)
	deltaX = vr::Random::real( 0.0f, deltaRatio );
	deltaY = vr::Random::real( 0.0f, deltaRatio );
	upperRightSample.initPrimaryRay( x + deltaX, y + deltaY );
	ctx->traceNearest( upperRightSample );
	vr::vec3f upperRightColor = upperRightSample.color;

	// Lower left (C)
	deltaX = vr::Random::real( -deltaRatio, 0.0f );
	deltaY = vr::Random::real( -deltaRatio, 0.0f );
	lowerLeftSample.initPrimaryRay( x + deltaX, y + deltaY );
	ctx->traceNearest( lowerLeftSample );
	vr::vec3f lowerLeftColor = lowerLeftSample.color;

	// Lower right (D)
	deltaX = vr::Random::real( 0.0f, deltaRatio );
	deltaY = vr::Random::real( -deltaRatio, 0.0f );
	lowerRightSample.initPrimaryRay( x + deltaX, y + deltaY );
	ctx->traceNearest( lowerRightSample );
	vr::vec3f lowerRightColor = lowerRightSample.color;

	if( recursionDepth < _maxRecursionDepth )
	{
		// If too much difference in sample values
		vr::vec3f ab = upperLeftColor - upperRightColor;
		ab.set( vr::abs( ab.r ), vr::abs( ab.g ), vr::abs( ab.b ) );

		vr::vec3f ac = upperLeftColor - lowerLeftColor;
		ac.set( vr::abs( ac.r ), vr::abs( ac.g ), vr::abs( ac.b ) );

		vr::vec3f ad = upperLeftColor - lowerRightColor;
		ad.set( vr::abs( ad.r ), vr::abs( ad.g ), vr::abs( ad.b ) );

		vr::vec3f bc = upperRightColor - lowerLeftColor;
		bc.set( vr::abs( bc.r ), vr::abs( bc.g ), vr::abs( bc.b ) );

		vr::vec3f bd = upperRightColor - lowerRightColor;
		bd.set( vr::abs( bd.r ), vr::abs( bd.g ), vr::abs( bd.b ) );

		vr::vec3f cd = upperLeftColor - lowerRightColor;
		cd.set( vr::abs( cd.r ), vr::abs( cd.g ), vr::abs( cd.b ) );

		if( ( ab.r > _epsilon ) || ( ab.g > _epsilon ) || ( ab.b > _epsilon ) ||
			( ac.r > _epsilon ) || ( ac.g > _epsilon ) || ( ac.b > _epsilon ) ||
			( ad.r > _epsilon ) || ( ad.g > _epsilon ) || ( ad.b > _epsilon ) ||
			( bc.r > _epsilon ) || ( bc.g > _epsilon ) || ( bc.b > _epsilon ) ||
			( bd.r > _epsilon ) || ( bd.g > _epsilon ) || ( bd.b > _epsilon ) ||
			( cd.r > _epsilon ) || ( cd.g > _epsilon ) || ( cd.b > _epsilon ) )
		{
			// Recursive supersample
			const float recDelta = 0.5f / ( (float)recursionDepth + 1.0f );

			vr::vec3f recUpperLeft;
			vr::vec3f recUpperRight;
			vr::vec3f recLowerLeft;
			vr::vec3f recLowerRight;

			adaptiveSupersample( x - recDelta, y + recDelta, recUpperLeft, recursionDepth + 1 );
			adaptiveSupersample( x + recDelta, y + recDelta, recUpperRight, recursionDepth + 1 );
			adaptiveSupersample( x - recDelta, y - recDelta, recLowerLeft, recursionDepth + 1 );
			adaptiveSupersample( x + recDelta, y - recDelta, recLowerRight, recursionDepth + 1 );

			// Average results
			resultColor = ( upperLeftColor * 0.125f ) + ( recUpperLeft * 0.125f ) + 
				          ( upperRightColor * 0.125f ) + ( recUpperRight * 0.125f ) + 
						  ( lowerLeftColor * 0.125f ) + ( recLowerLeft * 0.125f ) + 
						  ( lowerRightColor * 0.125f ) + ( recLowerRight * 0.125f );

			return;
		}
	}

	// Average results
	resultColor = ( upperLeftColor * 0.25f ) + ( upperRightColor * 0.25f ) + 
		          ( lowerLeftColor * 0.25f ) + ( lowerRightColor * 0.25f );
}
