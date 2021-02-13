#include <rt/Aabb.h>

using namespace rt;

Aabb::Aabb()
: minv( vr::Mathf::MAX_VALUE, vr::Mathf::MAX_VALUE, vr::Mathf::MAX_VALUE ),
  maxv( -vr::Mathf::MAX_VALUE, -vr::Mathf::MAX_VALUE, -vr::Mathf::MAX_VALUE )
{
}

void Aabb::buildFrom( const vr::vec3f* vertices, int32 vertexCount )
{
	// Check degenerate box
	if( vertexCount == 0 )
	{
		// Reset to default Aabb
		*this = Aabb();
		return;
	}
	
	// Get initial values
	minv = vertices[0];
	maxv = vertices[0];

	// For each vertex beyond first
	expandBy( vertices + 1, vertexCount - 1 );
}

void Aabb::expandBy( const vr::vec3f* vertices, int32 vertexCount )
{
	for( int32 i = 0; i < vertexCount; ++i )
	{
		expandBy( vertices[i] );
	}
}

void Aabb::expandBy( const vr::vec3f& vertex )
{
	if( vertex.x < minv.x )
		minv.x = vertex.x;
	else if( vertex.x > maxv.x )
		maxv.x = vertex.x;

	if( vertex.y < minv.y )
		minv.y = vertex.y;
	else if( vertex.y > maxv.y )
		maxv.y = vertex.y;

	if( vertex.z < minv.z )
		minv.z = vertex.z;
	else if( vertex.z > maxv.z )
		maxv.z = vertex.z;
}

void Aabb::expandBy( const Aabb& other )
{
	// Min vertex
	if( other.minv.x < minv.x )
		minv.x = other.minv.x;

	if( other.minv.y < minv.y )
		minv.y = other.minv.y;

	if( other.minv.z < minv.z )
		minv.z = other.minv.z;

	// Max vertex
	if( other.maxv.x > maxv.x )
		maxv.x = other.maxv.x;

	if( other.maxv.y > maxv.y )
		maxv.y = other.maxv.y;

	if( other.maxv.z > maxv.z )
		maxv.z = other.maxv.z;
}

void Aabb::scaleBy( float scale, float tolerance )
{
	float s;
	for( RTenum d = RT_AXIS_X; d <= RT_AXIS_Z; ++d )
	{
		if( isPlanar( d ) )
			s = tolerance;
		else
			s = vr::abs( maxv[d] - minv[d] ) * scale;

		minv[d] -= s;
		maxv[d] += s;
	}
}

bool Aabb::isPlanar( RTenum axis ) const
{
	return minv[axis] == maxv[axis];
}

bool Aabb::isDegenerate() const
{
	return ( minv.x > maxv.x ) || ( minv.y > maxv.y ) || ( minv.z > maxv.z );
}

float Aabb::computeSurfaceArea() const
{
	float a = maxv.x - minv.x;
	float b = maxv.y - minv.y;
	float c = maxv.z - minv.z;
	return a*b + a*c + b*c;
}

/*
*  Vertices are computed as follows:
*     7+------+6
*     /|     /|      y
*    / |    / |      |
*   / 3+---/--+2     |
* 4+------+5 /       *---x
*  | /    | /       /
*  |/     |/       z
* 0+------+1      
*
* Assumes array is pre-allocated!
*/
void Aabb::computeVertices( vr::vec3f* vertices ) const
{
	vertices[0].set( minv.x, minv.y, maxv.z );
	vertices[1].set( maxv.x, minv.y, maxv.z );
	vertices[2].set( maxv.x, minv.y, minv.z );
	vertices[3] = minv;
	vertices[4].set( minv.x, maxv.y, maxv.z );
	vertices[5] = maxv;
	vertices[6].set( maxv.x, maxv.y, minv.z );
	vertices[7].set( minv.x, maxv.y, minv.z );
}
