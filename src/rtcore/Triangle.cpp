#include <rt/Triangle.h>

using namespace rt;

TriDesc::TriDesc()
: material( NULL )
{
	// empty
}

void TriAccel::buildFrom( const vr::vec3f& v0, const vr::vec3f& v1, const vr::vec3f& v2 )
{
	// Compute edges and normal
	vr::vec3f b = v2 - v0;
	vr::vec3f c = v1 - v0;
	vr::vec3f normal = c.cross( b );

	// Determine projection dimension
	uint32 dim;
	
	if( ( vr::abs( normal.x ) > vr::abs( normal.y ) ) && ( vr::abs( normal.x ) > vr::abs( normal.z ) ) )
		dim = RT_AXIS_X;
	else if( vr::abs( normal.y ) > vr::abs( normal.z ) )
		dim = RT_AXIS_Y;
	else 
		dim = RT_AXIS_Z;

	k = dim;

	// Secondary dimensions
	uint32 u = ( k + 1 ) % 3;
	uint32 v = ( k + 2 ) % 3;

	// Store normal
	n_u = normal[u] / normal[k];
	n_v = normal[v] / normal[k];

	// Compute N'
	vr::vec3f nLine( normal );
	nLine *= 1.0f / normal[k];

	// Store plane distance
	n_d = v0.dot( nLine );

	// Compute constant for line ac
	float invDenom = 1.0f / ( b[u]*c[v] - b[v]*c[u] );
	b_nu = -b[v] * invDenom;
	b_nv =  b[u] * invDenom;
	b_d  = ( b[v]*v0[u] - b[u]*v0[v] ) * invDenom;

	// Compute constant for line ab
	c_nu =  c[v] * invDenom;
	c_nv = -c[u] * invDenom;
	c_d  = ( c[u]*v0[v] - c[v]*v0[u] ) * invDenom;
}

bool TriAccel::valid() const
{
	// Check for INFs and NaNs
	return !( vr::isInvalidNumber( n_u )  || vr::isInvalidNumber( n_v )  || vr::isInvalidNumber( n_d )  || 
		      vr::isInvalidNumber( b_nu ) || vr::isInvalidNumber( b_nv ) || vr::isInvalidNumber( b_d )  ||
		      vr::isInvalidNumber( c_nu ) || vr::isInvalidNumber( c_nv ) || vr::isInvalidNumber( c_d )  );
}
