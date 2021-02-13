#include <rtdb/ObjFileLoader.h>
#include <rtdb/FileManager.h>

#include <rt/Context.h>

#include <rtp/HeadlightMaterialColor.h>
#include <rtp/PhongMaterialColor.h>
#include <rtp/Texture2D.h>

#include <obj/objparser.h>
#include <obj/mtlparser.h>

#include <vr/timer.h>

#include <sstream>

using namespace rtdb;

class ObjAdapter : public sig::has_slots<>
{
public:
	ObjAdapter()
	{
		ctx = rt::Context::current();
		currentMaterial = NULL;
		imgLoader = NULL;
	}

	void connect( obj::objparser& parser )
	{
		// Connect signals to slots
		parser.errorSignal.connect( this, &ObjAdapter::error_slot );
		parser.commentSignal.connect( this, &ObjAdapter::comment_slot );
		parser.vertexSignal.connect( this, &ObjAdapter::vertex_slot );
		parser.normalSignal.connect( this, &ObjAdapter::normal_slot );
		parser.texcoordSignal.connect( this, &ObjAdapter::texcoord_slot );
		parser.faceBeginSignal.connect( this, &ObjAdapter::faceBegin_slot );
		parser.faceElementSignal.connect( this, &ObjAdapter::faceElement_slot );
		parser.faceEndSignal.connect( this, &ObjAdapter::faceEnd_slot );
		parser.objectNameSignal.connect( this, &ObjAdapter::objectName_slot );
		parser.groupNameSignal.connect( this, &ObjAdapter::groupName_slot );
		parser.materialLibSignal.connect( this, &ObjAdapter::materialLib_slot );
		parser.materialUseSignal.connect( this, &ObjAdapter::materialUse_slot );

		defaultMaterialId = ctx->createMaterials( 1 );

		rtp::HeadlightMaterialColor* mat = new rtp::HeadlightMaterialColor();
		ctx->setMaterial( defaultMaterialId, mat );
	}

	void connectMtl( obj::mtlparser& parser )
	{
		// Connect signals to slots
		parser.errorSignal.connect( this, &ObjAdapter::errorMtl_slot );
		parser.commentSignal.connect( this, &ObjAdapter::commentMtl_slot );
		parser.beginMaterialSignal.connect( this, &ObjAdapter::beginMaterial_slot );
		parser.ambientSignal.connect( this, &ObjAdapter::ambient_slot );
		parser.diffuseSignal.connect( this, &ObjAdapter::diffuse_slot );
		parser.specularSignal.connect( this, &ObjAdapter::specular_slot );
		parser.specularExpSignal.connect( this, &ObjAdapter::specularExp_slot );
		parser.opacitySignal.connect( this, &ObjAdapter::opacity_slot );
		parser.refractionIndexSignal.connect( this, &ObjAdapter::refractionIndex_slot );
		parser.textureAmbientSignal.connect( this, &ObjAdapter::textureAmbient_slot );
		parser.textureDiffuseSignal.connect( this, &ObjAdapter::textureDiffuse_slot );
		parser.textureSpecularSignal.connect( this, &ObjAdapter::textureSpecular_slot );
	}

	void error_slot( unsigned int lineNumber, const std::string& msg )
	{
		std::cout << "error in obj (" << lineNumber << "): " << msg << std::endl;
	}

	void comment_slot( unsigned int lineNumber, const std::string& msg )
	{
	}

	void vertex_slot( const obj::vec3d& v )
	{
		vertices.push_back( vr::vec3f( v.x, v.y, v.z ) );
	}

	void normal_slot( const obj::vec3d& v )
	{
		normals.push_back( vr::vec3f( v.x, v.y, v.z ) );
	}

	void texcoord_slot( const obj::vec3d& v )
	{
		texcoords.push_back( vr::vec3f( v.x, v.y, v.z ) );
	}

	void faceBegin_slot( unsigned int numElements )
	{
		if( normals.empty() )
		{
			std::cout << "No normals found, aborting execution." << std::endl;
			exit( 1 );
		}

		// Save bindings
		ctx->getBindingStack().pushAllAttrib();
		ctx->getBindingStack().top().colorBinding = RT_BIND_PER_MATERIAL;

		if( texcoords.empty() )
			ctx->getBindingStack().top().textureBinding = RT_BIND_PER_MATERIAL;
		else
			ctx->getBindingStack().top().textureBinding = RT_BIND_PER_VERTEX;

		// Bind default material if none provided
		if( currentMaterial == NULL )
		{
			ctx->bindMaterial( defaultMaterialId );
			printf( "Warning: no material bound in OBJ, using default material.\n" );
		}

		// Case triangle
		if( numElements == 3 )
		{
			ctx->beginPrimitive( RT_TRIANGLES );
		}
		// Case face
		else if( numElements > 3 )
		{
			ctx->beginPrimitive( RT_POLYGON );
		}
		else
		{
			std::cout << "Too few elements in face, aborting execution." << std::cout;
			exit( 1 );
		}
	}

	void faceElement_slot( const obj::face_index& idx )
	{
		if( idx.texCoordIdx > 0 )
			ctx->setTexCoord( texcoords[idx.texCoordIdx-1].ptr );

		if( idx.normalIdx > 0 )
			ctx->setNormal( normals[idx.normalIdx-1].ptr );

		if( idx.vertexIdx > 0 )
			ctx->addVertex( vertices[idx.vertexIdx-1].ptr );
	}

	void faceEnd_slot()
	{
		ctx->endPrimitive();
		ctx->getBindingStack().popAllAttrib();
	}

	void objectName_slot( const std::string& name )
	{
	}

	void groupName_slot( const std::string& name )
	{
	}

	void materialLib_slot( const std::string& filename )
	{
		obj::mtlparser mtlParser;
		connectMtl( mtlParser );

		mtlParser.parse( ( filePath + filename ).c_str() );
	}

	void materialUse_slot( const std::string& name )
	{
		std::map<std::string, uint32>::const_iterator itr = materialMap.find( name );
		if( itr == materialMap.end() )
		{
			std::cout << "error: tried to use a material name that does not exist." << std::endl;
			currentMaterial = NULL;
			return;
		}

		ctx->bindMaterial( itr->second );
		currentMaterial = (rtp::PhongMaterialColor*)rt::Context::current()->getMaterial( itr->second );
	}

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	
	void errorMtl_slot( unsigned int lineNumber, const std::string& msg )
	{
		std::cout << "error in mtl (" << lineNumber << "): " << msg << std::endl;
	}

	void commentMtl_slot( unsigned int lineNumber, const std::string& msg )
	{
	}

	void  beginMaterial_slot( const std::string& name )
	{
		currentMaterial = new rtp::PhongMaterialColor();

		uint32 matId = ctx->createMaterials( 1 );
		ctx->setMaterial( matId, currentMaterial );

		materialMap[name] = matId;
	}

	void ambient_slot( const obj::vec3d& color )
	{
		currentMaterial->setAmbient( color.x, color.y, color.z );
	}

	void diffuse_slot( const obj::vec3d& color )
	{
		currentMaterial->setDiffuse( color.x, color.y, color.z );
	}

	void specular_slot( const obj::vec3d& color )
	{
		currentMaterial->setSpecular( color.x, color.y, color.z );
	}

	void specularExp_slot( double value )
	{
		currentMaterial->setSpecularExponent( value );
	}

	void opacity_slot( double value )
	{
		currentMaterial->setOpacity( value );
	}

	void refractionIndex_slot( double value )
	{
		currentMaterial->setRefractionIndex( value );
	}

	void textureAmbient_slot( const std::string& filename )
	{
	}

	void textureDiffuse_slot( const std::string& filename )
	{
		if( imgLoader == NULL )
			return;

		bool ok = imgLoader->loadImage( ( filePath + filename ).c_str() );

		if( !ok )
		{
			std::cout << "error loading texture image '" << filename << "', skipping texture." << std::endl;
			return;
		}

		// Setup texture
		rtp::Texture2D* tex = new rtp::Texture2D();
		tex->setEnvMode( RT_MODULATE );
		tex->setFilter( RT_LINEAR );
		tex->setWrapS( RT_REPEAT );
		tex->setWrapT( RT_REPEAT );
		tex->setTextureImage2D( imgLoader->getWidth(), imgLoader->getHeight(), imgLoader->getImage() );

		uint32 id = ctx->createTextures( 1 );
		ctx->setTexture( id, tex );

		currentMaterial->setTexture( id );
	}

	void textureSpecular_slot( const std::string& filename )
	{
	}

	rt::Context* ctx;

	std::string filePath;
	IImageLoader* imgLoader;

	std::vector<vr::vec3f> vertices;
	std::vector<vr::vec3f> normals;
	std::vector<vr::vec3f> texcoords;

	uint32 defaultMaterialId;

	std::map<std::string, uint32> materialMap;
	rtp::PhongMaterialColor* currentMaterial;
};

ObjFileLoader::ObjFileLoader()
{
	_imgLoader = NULL;
}

void ObjFileLoader::registerSupportedExtensions()
{
	FileManager::instance()->registerLoaderToExtension( this, ".obj" );
}

bool ObjFileLoader::loadGeometry( const vr::String& filename )
{
	obj::objparser objParser;

	ObjAdapter adapter;
	adapter.connect( objParser );
	adapter.filePath = getFilePath( filename ).data();
	adapter.imgLoader = _imgLoader;

	rt::Context* ctx = rt::Context::current();

	uint32 geomId = ctx->createGeometries( 1 );
	ctx->beginGeometry( geomId );

	// Read file and send geometry data to ray tracer
	objParser.parse( filename.data() );

	vr::Timer timer;
	timer.restart();
	ctx->endGeometry();

	printf( "cpu grid build time: %.6f secs\n", timer.elapsed() );

	return true;
}

void ObjFileLoader::setImageLoader( IImageLoader* loader )
{
	_imgLoader = loader;
}
