#include <rt/ICamera.h>

using namespace rt;

void ICamera::translate( float x, float y, float z )
{
	// avoid warnings
	x;y;z;
}

void ICamera::rotate( float radians, uint32 axis )
{
	// avoid warnings
	radians;axis;
}

void ICamera::setLookAt( float eyeX, float eyeY, float eyeZ, 
	 				  float centerX, float centerY, float centerZ, 
	                  float upX, float upY, float upZ )
{
	// avoid warnings
	eyeX;eyeY;eyeZ;centerX;centerY;centerZ;upX;upY;upZ;
}

void ICamera::setPerspective( float fovY, float zNear, float zFar )
{
	// avoid warnings
	fovY;zNear;zFar;
}

void ICamera::setViewport( uint32 width, uint32 height )
{
	// avoid warnings
	width;height;
}

void ICamera::getViewport( uint32& width, uint32& height )
{
	width = 0;
	height = 0;
}

void ICamera::getRayOrigin( vr::vec3f& origin, float x, float y )
{
	origin.set( 0, 0, 0 );
}

void ICamera::getRayDirection( vr::vec3f& dir, float x, float y )
{
	dir.set( 0, 0, 0 );
}
