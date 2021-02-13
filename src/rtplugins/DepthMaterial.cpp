#include <rtp/DepthMaterial.h>

using namespace rtp;

void DepthMaterial::shade( rt::Sample& sample )
{
	sample.computeHitPosition();
	float distance = ( sample.hitPosition - sample.ray.orig ).length();
	sample.color.set( distance, distance, distance );
}
