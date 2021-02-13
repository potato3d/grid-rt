#include <rtgl/GpuObjLoader.h>

#include <obj/objparser.h>
#include <obj/mtlparser.h>

#include <map>
#include <string>

using namespace rtgl;

class ObjAnimAdapter : public sig::has_slots<>
{
public:
	ObjAnimAdapter( ObjAnimation* _animation, rtdb::IImageLoader* _imgLoader )
	{
		animation = _animation;
		imgLoader = _imgLoader;
	}

	void setFilePath( const char* path )
	{
		filePath = path;
	}

	void beginRead()
	{
		animation->frameObjs.push_back( new KeyFrameObj() );

		currentMatId = -1;
		currentTexId = -1;

		vertices.clear();
		normals.clear();
		texcoords.clear();
		faceElements.clear();
	}

	void connect( obj::objparser& parser )
	{
		// Connect signals to slots
		parser.errorSignal.connect( this, &ObjAnimAdapter::error_slot );
		parser.vertexSignal.connect( this, &ObjAnimAdapter::vertex_slot );
		parser.normalSignal.connect( this, &ObjAnimAdapter::normal_slot );
		parser.texcoordSignal.connect( this, &ObjAnimAdapter::texcoord_slot );
		parser.faceBeginSignal.connect( this, &ObjAnimAdapter::faceBegin_slot );
		parser.faceElementSignal.connect( this, &ObjAnimAdapter::faceElement_slot );
		parser.faceEndSignal.connect( this, &ObjAnimAdapter::faceEnd_slot );
		parser.materialLibSignal.connect( this, &ObjAnimAdapter::materialLib_slot );
		parser.materialUseSignal.connect( this, &ObjAnimAdapter::materialUse_slot );
	}

	void connectMtl( obj::mtlparser& parser )
	{
		// Connect signals to slots
		parser.errorSignal.connect( this, &ObjAnimAdapter::errorMtl_slot );
		parser.beginMaterialSignal.connect( this, &ObjAnimAdapter::beginMaterial_slot );
		parser.diffuseSignal.connect( this, &ObjAnimAdapter::diffuse_slot );
		parser.textureDiffuseSignal.connect( this, &ObjAnimAdapter::textureDiffuse_slot );
	}

	void error_slot( unsigned int lineNumber, const std::string& msg )
	{
		std::cout << "error in obj (" << lineNumber << "): " << msg << std::endl;
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
		texcoords.push_back( vr::vec2f( v.x, v.y ) );
	}

	void faceBegin_slot( unsigned int numElements )
	{
		if( normals.empty() )
		{
			std::cout << "No normals found, aborting execution." << std::endl;
			exit( 1 );
		}

		faceElements.clear();
	}

	void faceElement_slot( const obj::face_index& idx )
	{
		faceElements.push_back( idx );
	}

	void addElem( const obj::face_index& elem )
	{
		if( elem.texCoordIdx > 0 )
		{
			animation->frameObjs.back()->texcoords.push_back( texcoords[elem.texCoordIdx-1] );
		}
		else
		{
			animation->frameObjs.back()->texcoords.push_back( vr::vec2f( 0.0f, 0.0f ) );
			//printf( "default texcoord added!\n" );
		}

		if( elem.normalIdx > 0 )
		{
			animation->frameObjs.back()->normals.push_back( normals[elem.normalIdx-1] );
		}
		else
		{
			animation->frameObjs.back()->normals.push_back( vr::vec3f( 0.0f ) );
			//printf( "default normal added!\n" );
		}

		if( elem.vertexIdx > 0 )
		{
			animation->frameObjs.back()->vertices.push_back( vertices[elem.vertexIdx-1] );
		}
		else
		{
			animation->frameObjs.back()->vertices.push_back( vr::vec3f( 0.0f ) );
			//printf( "default vertex added!\n" );
		}
	}

	void addCurrentMat()
	{
		if( currentMatId >= 0 )
			animation->frameObjs.back()->materials.push_back( animation->materials[currentMatId] );
		else
			animation->frameObjs.back()->materials.push_back( MaterialAttrib() );
	}

	void faceEnd_slot()
	{
		// Expand face to store individual triangle vertices, normals and texcoords
		for( unsigned int i = 1; i < faceElements.size() - 1; ++i )
		{
			// Add triangle
			addElem( faceElements[0] );
			addElem( faceElements[i] );
			addElem( faceElements[i+1] );

			// Copy same current material for each triangle
			addCurrentMat();
		}
	}

	void materialLib_slot( const std::string& filename )
	{
		obj::mtlparser mtlParser;
		connectMtl( mtlParser );

		mtlParser.parse( ( filePath + filename ).c_str() );
	}

	void materialUse_slot( const std::string& name )
	{
		std::map<std::string, int>::const_iterator itr = materialMap.find( name );
		if( itr == materialMap.end() )
		{
			std::cout << "error: tried to use a material name that does not exist." << std::endl;
			currentMatId = -1;
			return;
		}

		currentMatId = itr->second;
	}

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	void errorMtl_slot( unsigned int lineNumber, const std::string& msg )
	{
		std::cout << "error in mtl (" << lineNumber << "): " << msg << std::endl;
	}

	void beginMaterial_slot( const std::string& name )
	{
		// If material name already exists, no need to redefine it
		std::map<std::string, int>::const_iterator itr = materialMap.find( name );
		if( itr != materialMap.end() )
		{
			currentMatId = itr->second;
			return;
		}

		// If new material, create and store id in map
		animation->materials.push_back( MaterialAttrib() );
		currentMatId = animation->materials.size() - 1;
		materialMap[name] = currentMatId;
	}

	void diffuse_slot( const obj::vec3d& color )
	{
		animation->materials[currentMatId].diffuse.set( color.x, color.y, color.z );
	}

	void textureDiffuse_slot( const std::string& filename )
	{
		if( imgLoader == NULL )
			return;

		// If texture name already exists, no need to redefine it
		std::map<std::string, int>::const_iterator itr = textureMap.find( filename );
		if( itr != textureMap.end() )
		{
			animation->materials[currentMatId].textureId = itr->second;
			return;
		}

		bool ok = imgLoader->loadImage( ( filePath + filename ).c_str() );

		if( !ok )
		{
			std::cout << "error loading texture image '" << filename << "', skipping texture." << std::endl;
			return;
		}

		// Setup texture
		Tex2D tex;
		tex.w = imgLoader->getWidth();
		tex.h = imgLoader->getHeight();
		// RGBA
		tex.texels.insert( tex.texels.end(), imgLoader->getImage(), imgLoader->getImage() + tex.w * tex.h * 4 );

		animation->textures.push_back( tex );
		int texId = animation->textures.size() - 1;

		textureMap[filename] = texId;
		animation->materials[currentMatId].textureId = texId;
	}

	ObjAnimation* animation;

	std::vector<obj::face_index> faceElements;

	std::vector<vr::vec3f> vertices;
	std::vector<vr::vec3f> normals;
	std::vector<vr::vec2f> texcoords;

	std::map<std::string, int> materialMap;
	int currentMatId;

	std::map<std::string, int> textureMap;
	int currentTexId;

	rtdb::IImageLoader* imgLoader;

	std::string filePath;
};

void ObjAnimation::loadAnimation( const vr::StringList& filenames, rtdb::IImageLoader* imgLoader )
{
	ObjAnimAdapter adapter( this, imgLoader );
	obj::objparser objParser;
	adapter.connect( objParser );

	bbox = rt::Aabb();

	for( unsigned int i = 0; i < filenames.size(); ++i )
	{
		std::cout << "loading file: " << filenames[i] << std::endl;

		adapter.beginRead();

		adapter.setFilePath( rtdb::getFilePath( filenames[i] ).data() );

		objParser.parse( filenames[i].data() );

		bbox.expandBy( &frameObjs.back()->vertices[0], frameObjs.back()->vertices.size() );

		frameObjs.back()->numTriangles = frameObjs.back()->vertices.size() / 3;
	}
}

void ObjAnimation::printStats()
{
	if( !frameObjs.empty() )
		std::cout << "num triangles:  " << frameObjs[0]->vertices.size() / 3 << std::endl;

	std::cout <<	 "num key frames: " << frameObjs.size() << std::endl;
	std::cout <<	 "num materials:  " << materials.size() << std::endl;
	std::cout <<	 "num textures:   " << textures.size() << std::endl;
}
