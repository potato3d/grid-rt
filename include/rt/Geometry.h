#ifndef _RT_GEOMETRY_H_
#define _RT_GEOMETRY_H_

#include <rt/common.h>
#include <rt/Triangle.h>
#include <rt/IAccStruct.h>

namespace rt {

class Geometry : public vr::RefCounted
{
public:
	void clear();

	// vertexIdx must be 0, 1 or 2
	inline const vr::vec3f& getVertex( uint32 t, uint32 vertexIdx ) const;
	inline const vr::vec3f& getNormal( uint32 t, uint32 vertexIdx ) const;
	inline const vr::vec3f& getColor( uint32 t, uint32 vertexIdx ) const;
	inline const vr::vec3f& getTexCoords( uint32 t, uint32 vertexIdx ) const;

	vr::ref_ptr<rt::IAccStruct> accStruct;
	std::vector<TriAccel>        triAccel;
	std::vector<TriDesc>         triDesc;
	std::vector<vr::vec3f>       vertices;
	std::vector<vr::vec3f>       normals;
	std::vector<vr::vec3f>       colors;
	std::vector<vr::vec3f>       texCoords;

private:
	template<typename T>
	inline const vr::vec3f& getVertexData( T& data, uint32 t, uint32 vertexIdx ) const;
};

inline const vr::vec3f& Geometry::getVertex( uint32 t, uint32 vertexIdx ) const
{
	return getVertexData( vertices, t, vertexIdx );
}

inline const vr::vec3f& Geometry::getNormal( uint32 t, uint32 vertexIdx ) const
{
	return getVertexData( normals, t, vertexIdx );
}

inline const vr::vec3f& Geometry::getColor( uint32 t, uint32 vertexIdx ) const
{
	return getVertexData( colors, t, vertexIdx );
}

inline const vr::vec3f& Geometry::getTexCoords( uint32 t, uint32 vertexIdx ) const
{
	return getVertexData( texCoords, t, vertexIdx );
}

// Private
template<typename T>
inline const vr::vec3f& Geometry::getVertexData( T& data, uint32 t, uint32 vertexIdx ) const
{
	return data[triDesc[t].v[vertexIdx]];
}

} // namespace rt

#endif // _RT_GEOMETRY_H_
