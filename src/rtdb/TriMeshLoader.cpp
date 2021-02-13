#include <rtdb/TriMeshLoader.h>
#include <rtdb/FileManager.h>
#include <rt/Context.h>
#include <rtp/HeadlightMaterial.h>
#include <rtp/HeadlightMaterialColor.h>
#include <TriMesh.h>
#include <fstream>

using namespace rtdb;

void TriMeshLoader::registerSupportedExtensions()
{
	//FileManager::instance()->registerLoaderToExtension( this, ".obj" );
	FileManager::instance()->registerLoaderToExtension( this, ".ply" );
}

bool TriMeshLoader::loadGeometry( const vr::String& filename )
{
	TriMesh* mesh = TriMesh::read( filename.data() );
	if( mesh == NULL )
		return false;

	// Require mesh attributes
	mesh->need_faces();
	mesh->need_normals();
	bool haveColors = !mesh->colors.empty();

	rt::Context* ctx = rt::Context::current();
	rt::IMaterial* mat;

	if( haveColors )
	{
		mat = new rtp::HeadlightMaterial();
		ctx->getBindingStack().pushAllAttrib();
		ctx->getBindingStack().top().colorBinding = RT_BIND_PER_VERTEX;
	}
	else
	{
		mat = new rtp::HeadlightMaterialColor();
	}

	uint32 id = ctx->createMaterials( 1 );
	ctx->setMaterial( id, mat );
	ctx->bindMaterial( id );

	uint32 geometryId = ctx->createGeometries( 1 );
	ctx->beginGeometry( geometryId );

	// Load geometry data
	ctx->beginPrimitive( RT_TRIANGLES );
	for( uint32 i = 0; i < mesh->faces.size(); ++i )
	{
		const TriMesh::Face& face = mesh->faces[i];

		// Vertex 0
		if( haveColors )
			ctx->setColor( mesh->colors[face.v[0]].begin() );

		ctx->setNormal( mesh->normals[face.v[0]].begin() );
		ctx->addVertex( mesh->vertices[face.v[0]].begin() );

		// Vertex 1
		if( haveColors )
			ctx->setColor( mesh->colors[face.v[0]].begin() );

		ctx->setNormal( mesh->normals[face.v[1]].begin() );
		ctx->addVertex( mesh->vertices[face.v[1]].begin() );

		// Vertex 2
		if( haveColors )
			ctx->setColor( mesh->colors[face.v[0]].begin() );

		ctx->setNormal( mesh->normals[face.v[2]].begin() );
		ctx->addVertex( mesh->vertices[face.v[2]].begin() );
	}

	ctx->endPrimitive();

	ctx->endGeometry();

	if( haveColors )
		ctx->getBindingStack().popAllAttrib();

	delete mesh;
	return true;
}
