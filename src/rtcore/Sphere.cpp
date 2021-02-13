#include <rt/Sphere.h>

using namespace rt;

bool Sphere::computeFrom( const vr::vec3f& a, const vr::vec3f& b, const vr::vec3f& c )
{
	vr::vec3f ab = b - a;
	vr::vec3f ac = c - a;

	float dotABAB = ab.dot( ab );
	float dotABAC = ab.dot( ac );
	float dotACAC = ac.dot( ac );

	float d = 2.0f*(dotABAB*dotACAC - dotABAC*dotABAC);

	// TODO: 
	//if( vr::abs( d ) < vr::Mathf::ZERO_TOLERANCE )
	if( d == 0.0f )
	{
		printf( "Sphere (error): triangle vertices are co-linear!\n" );
		return false;
	}

	float s = (dotABAB*dotACAC - dotACAC*dotABAC) / d;
	float t = (dotACAC*dotABAB - dotABAB*dotABAC) / d;

	vr::vec3f tp = a;

	// s controls height over AC, t over AB, (1-s-t) over BC
	if( s <= 0.0f )
	{
		center = (a + c)*0.5f;
	}
	else if( t <= 0.0f )
	{
		center = (a + b)*0.5f;
	}
	else if( s + t >= 1.0f )
	{
		center = (b + c)*0.5f;
		tp = b;
	}
	else
	{
		center = a + ab*s + ac*t;
	}

	radius = ( center - tp ).length();

	return true;
}
