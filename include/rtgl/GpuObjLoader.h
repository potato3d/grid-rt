#ifndef _RTGL_GPUOBJLOADER_H_
#define _RTGL_GPUOBJLOADER_H_

#include <rtdb/ObjFileLoader.h>
#include <rt/Aabb.h>
#include <vr/vec3.h>
#include <vr/vec2.h>

namespace rtgl {

class Tex2D
{
public:
	Tex2D()
	{
		w = 0;
		h = 0;
	}

	int w;
	int h;
	std::vector<unsigned char> texels;
};

class MaterialAttrib
{
public:
	MaterialAttrib()
	{
		diffuse.set( 0.7f );
		textureId = 0.0f;
	}

	vr::vec3f diffuse;
	float     textureId;
};

class KeyFrameObj : public vr::RefCounted
{
public:
	unsigned int numTriangles;

	// Size = numTriangles * 3
	std::vector<vr::vec3f> vertices;
	std::vector<vr::vec3f> normals;
	std::vector<vr::vec2f> texcoords;
	std::vector<MaterialAttrib> materials;
};

class ObjAnimation
{
public:
	void loadAnimation( const vr::StringList& filenames, rtdb::IImageLoader* imgLoader );

	void printStats();

	// Size = numKeyFrames
	std::vector< vr::ref_ptr<KeyFrameObj> > frameObjs;

	// Size = numMaterials
	std::vector<MaterialAttrib> materials;

	// Size = numTextures
	std::vector<Tex2D> textures;

	// Scene info
	rt::Aabb bbox;
};

}

#endif
