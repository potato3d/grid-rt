#include <rtp/HeadlightMaterial.h>

using namespace rtp;

HeadlightMaterial::HeadlightMaterial()
{
	_ambient.set( 0.1f, 0.1f, 0.1f );
}

void HeadlightMaterial::shade( rt::Sample& sample )
{
	sample.ray.dir.normalize();
	sample.computeShadingNormal();
	sample.computeShadingColor();

	const float nDotD = -( sample.normal.dot( sample.ray.dir ) );

	sample.color = ( _ambient + ( sample.color - _ambient ) * nDotD ) * sample.color;
}
