#ifndef _RT_PRIMITIVEBUILDER_H_
#define _RT_PRIMITIVEBUILDER_H_

#include <rt/common.h>
#include <rt/Geometry.h>
#include <rt/Transform.h>
#include <rt/AttributeBinding.h>
#include <rt/IMaterial.h>

namespace rt {

class PrimitiveBuilder
{
public:
	PrimitiveBuilder();

	void begin( RTenum primitiveType, Geometry* geometry );

	void setMaterial( rt::IMaterial* material );
	Transform& getTransform();
	AttributeBinding& getBindings();

	void setColor( float r, float g, float b );
	void setColor( float const * const color );
	void setTexCoord( float s, float t, float p );
	void setTexCoord( float const * const texCoord );

	void setNormal( float x, float y, float z );
	void setNormal( float const * const normal );

	void addVertex( float x, float y, float z );
	void addVertex( float const * const vertex );

	void end();

private:
	void storeValidTriangle( const TriDesc& triangle, TriAccel& accel );
	void updateTriangles();
	void updateTriangleStrip();
	void updatePolygonOrFan();
	
	uint32 _primitiveType;
	Geometry* _geometry;
	rt::IMaterial* _material;
	uint32 _startVertex;
	Transform _transform;
	AttributeBinding _bindings;

	vr::vec3f _currentColor;
	vr::vec3f _currentNormal;
	vr::vec3f _currentTexCoord;
};

} // namespace rt

#endif // _RT_PRIMITIVEBUILDER_H_
