#include <rt/PrimitiveBuilder.h>

using namespace rt;

PrimitiveBuilder::PrimitiveBuilder()
{
	_primitiveType = RT_TRIANGLES;
	_geometry = NULL;
	_material = NULL;
	_startVertex = 0;
	_bindings.reset();

	_currentColor.set( 1.0f, 1.0f, 1.0f );
	_currentNormal.set( 0.0f, 0.0f, 1.0f );
	_currentTexCoord.set( 0.0f, 0.0f, 0.0f );
}

void PrimitiveBuilder::begin( RTenum primitiveType, Geometry* geometry )
{
	_primitiveType = primitiveType;
	_geometry = geometry;
	_startVertex = _geometry->vertices.size();
}

void PrimitiveBuilder::setMaterial( rt::IMaterial* material )
{
	_material = material;
}

Transform& PrimitiveBuilder::getTransform()
{
	return _transform;
}

AttributeBinding& PrimitiveBuilder::getBindings()
{
	return _bindings;
}

void PrimitiveBuilder::setColor( float r, float g, float b )
{
	_currentColor.set( r, g, b );
}

void PrimitiveBuilder::setColor( float const * const color )
{
	setColor( color[0], color[1], color[2] );
}

void PrimitiveBuilder::setTexCoord( float s, float t, float p )
{
	_currentTexCoord.set( s, t, p );
}

void PrimitiveBuilder::setTexCoord( float const * const texCoord )
{
	setTexCoord( texCoord[0], texCoord[1], texCoord[2] );
}

void PrimitiveBuilder::setNormal( float x, float y, float z )
{
	_currentNormal.set( x, y, z );
}

void PrimitiveBuilder::setNormal( float const * const normal )
{
	setNormal( normal[0], normal[1], normal[2] );
}

void PrimitiveBuilder::addVertex( float x, float y, float z )
{
	vr::vec3f transformedVertex( x, y, z );
	_transform.transformVertex( transformedVertex );
	_geometry->vertices.push_back( transformedVertex );

	if( _bindings.normalBinding == RT_BIND_PER_VERTEX )
	{
		vr::vec3f transformedNormal( _currentNormal );
		_transform.transformNormal( transformedNormal );
		transformedNormal.normalize();
		_geometry->normals.push_back( transformedNormal );
	}

	if( _bindings.colorBinding == RT_BIND_PER_VERTEX )
		_geometry->colors.push_back( _currentColor );

	if( _bindings.textureBinding == RT_BIND_PER_VERTEX )
		_geometry->texCoords.push_back( _currentTexCoord );
}

void PrimitiveBuilder::addVertex( float const * const vertex )
{
	addVertex( vertex[0], vertex[1], vertex[2] );
}

void PrimitiveBuilder::end()
{
	switch( _primitiveType )
	{
	case RT_TRIANGLES:
		updateTriangles();
		break;
	case RT_TRIANGLE_STRIP:
		updateTriangleStrip();
		break;
	case RT_TRIANGLE_FAN:
	case RT_POLYGON:
		updatePolygonOrFan();
		break;
	default:
		// TODO: warning message
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
// Private
//////////////////////////////////////////////////////////////////////////

void PrimitiveBuilder::storeValidTriangle( const TriDesc& triangle, TriAccel& accel )
{
	if( !accel.valid() )
		return;

	accel.triangleId = _geometry->triDesc.size();
	_geometry->triDesc.push_back( triangle );
	_geometry->triAccel.push_back( accel );
}

void PrimitiveBuilder::updateTriangles()
{
	uint32 idx = _startVertex;
	std::vector<vr::vec3f>& vertices = _geometry->vertices;
	uint32 limit = vertices.size();
	TriDesc triangle;

	while( idx < limit )
	{
		// Setup TriDesc
		triangle.v[0] = idx;
		++idx;
		triangle.v[1] = idx;
		++idx;
		triangle.v[2] = idx;
		++idx;
		triangle.material = _material;

		// Setup TriAccel
		TriAccel accel;
		accel.buildFrom( vertices[triangle.v[0]], vertices[triangle.v[1]], vertices[triangle.v[2]] );

		// Only store valid triangles
		storeValidTriangle( triangle, accel );
	}
}

void PrimitiveBuilder::updateTriangleStrip()
{
	int32 idx = (int32)_startVertex;
	std::vector<vr::vec3f>& vertices = _geometry->vertices;
	int32 limit = (int32)vertices.size() - 2;
	TriDesc triangle;
	bool inverted = false;

	// Vertices v1 and v2 are shared between current triangle and next one
	while( idx < limit )
	{
		// Setup TriDesc
		if( inverted )
		{
			triangle.v[0] = idx + 2;
			triangle.v[1] = idx + 1;
			triangle.v[2] = idx;
		}
		else
		{
			triangle.v[0] = idx;
			triangle.v[1] = idx + 1;
			triangle.v[2] = idx + 2;
		}
		triangle.material = _material;

		// Setup TriAccel
		TriAccel accel;
		accel.buildFrom( vertices[triangle.v[0]], vertices[triangle.v[1]], vertices[triangle.v[2]] );

		// Only store valid triangles
		storeValidTriangle( triangle, accel );

		// To next triangle
		++idx;
		inverted ^= true;
	}
}

void PrimitiveBuilder::updatePolygonOrFan()
{
	uint32 idx = _startVertex;
	std::vector<vr::vec3f>& vertices = _geometry->vertices;
	int32 limit = ( (int32)vertices.size() - idx ) - 2;
	TriDesc triangle;

	// All triangles contain first vertex
	for( int32 i = 1; i <= limit; ++i )
	{
		// Setup TriDesc
		triangle.v[0] = _startVertex;
		triangle.v[1] = _startVertex + i;
		triangle.v[2] = _startVertex + i + 1;
		triangle.material = _material;

		// Setup TriAccel
		TriAccel accel;
		accel.buildFrom( vertices[triangle.v[0]], vertices[triangle.v[1]], vertices[triangle.v[2]] );

		// Only store valid triangles
		storeValidTriangle( triangle, accel );
	}
}
