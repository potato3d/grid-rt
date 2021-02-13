#include <rt/Geometry.h>

using namespace rt;

void Geometry::clear()
{
	if( accStruct.valid() )
		accStruct->clear();

	vr::vectorFreeMemory( triAccel );
	vr::vectorFreeMemory( triDesc );
	vr::vectorFreeMemory( vertices );
	vr::vectorFreeMemory( normals );
	vr::vectorFreeMemory( colors );
	vr::vectorFreeMemory( texCoords );
}
