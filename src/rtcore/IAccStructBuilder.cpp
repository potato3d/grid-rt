#include <rt/IAccStructBuilder.h>

using namespace rt;

void IAccStructBuilder::buildGeometry( rt::Geometry* geometry )
{
	rt::Aabb bbox;
	
	if( !geometry->vertices.empty() )
		bbox.buildFrom( &geometry->vertices[0], geometry->vertices.size() );

	geometry->accStruct = new IAccStruct();
	geometry->accStruct->setBoundingBox( bbox );
}

IAccStruct* IAccStructBuilder::buildInstance( const std::vector<rt::Instance> instances )
{
	// Default degenerate box
	rt::Aabb bbox;
	vr::vec3f boxVertices[8];

	// Compute general bounding box of entire scene
	for( uint32 i = 0; i < instances.size(); ++i )
	{
		const rt::Instance& instance = instances[i];
		bbox.expandBy( instances[i].bbox );
	}

	IAccStruct* st = new IAccStruct;
	st->setBoundingBox( bbox );

	return st;
}
