#include <rtp/PinholeCamera.h>
#include <vr/mat4.h>

using namespace rtp;

PinholeCamera::PinholeCamera()
: ICamera()
{
	_dirty = true;
	_position.set( 0, 0, 0 );
	_fovy = 60.0f;
	_zNear = 1.0f;
	_zFar = 1000.0f;
	_screenWidth = 400;
	_screenHeight = 300;
	_axisX = vr::vec3f::UNIT_X();
	_axisY = vr::vec3f::UNIT_Y();
	_axisZ = vr::vec3f::UNIT_Z();
	_orientation.makeRotation( 0, 1, 0, 0 );
	continuousTranslation = vr::vec3f::ZERO();
}

void PinholeCamera::newFrame()
{
	if( continuousTranslation.x != 0.0f || continuousTranslation.y != 0.0f || continuousTranslation.z != 0.0f )
		translate( continuousTranslation.x, continuousTranslation.y, continuousTranslation.z );

	if( !_dirty )
		return;

	_dirty = false;

	_axisX = vr::vec3f::UNIT_X();
	_axisY = vr::vec3f::UNIT_Y();
	_axisZ = vr::vec3f::UNIT_Z();

	_orientation.transform( _axisX );
	_orientation.transform( _axisY );
	_orientation.transform( _axisZ );

	// Compute the near plane origin
	float sz = _zNear;
	float sy = sz * tan( vr::toRadians( _fovy * 0.5f ) );
	float sx = ( sy * _screenWidth ) / _screenHeight;
	_nearOrigin = _position + _axisX * -sx + _axisY * -sy + _axisZ * -sz;

	// Compute the near plane axis
	_nearU = _axisX * ( 2.0f * sx );
	_nearV = _axisY * ( 2.0f * sy );

	// Pre-compute data needed for ray generation
	_invWidth = 1.0f / _screenWidth;
	_invHeight = 1.0f / _screenHeight;
	_baseDir = _nearOrigin - _position;
}

void PinholeCamera::translate( float x, float y, float z )
{
	vr::vec3f translation( x, y, z );
	_orientation.transform( translation );
	_position += translation;
	_dirty = true;
}

void PinholeCamera::rotate( float radians, RTenum axis )
{
	vr::quatf q;

	switch( axis )
	{
	case RT_AXIS_X:
		q.makeRotation( radians, _axisX );
		break;

	case RT_AXIS_Y:
		q.makeRotation( radians, _axisY );
		break;

	case RT_AXIS_Z:
		q.makeRotation( radians, _axisZ );
		break;

	default:
		return;
	}

	_orientation *= q;
	_dirty = true;
}

void PinholeCamera::setLookAt( float eyeX, float eyeY, float eyeZ, 
					           float centerX, float centerY, float centerZ, 
					           float upX, float upY, float upZ )
{
	_position.set( eyeX, eyeY, eyeZ );
	vr::vec3f center( centerX, centerY, centerZ );
	vr::vec3f up( upX, upY, upZ );

	vr::mat4f lookAtMatrix;
	lookAtMatrix.makeLookAt( _position, center, up );
	lookAtMatrix.getRotation( _orientation );
	_dirty = true;
}

void PinholeCamera::setPerspective( float fovY, float zNear, float zFar )
{
	_fovy  = fovY;
	_zNear = zNear;
	_zFar  = zFar;
	_dirty = true;
}

void PinholeCamera::setViewport( uint32 width, uint32 height )
{
	_screenWidth = width;
	_screenHeight = height;
	_dirty = true;
}

void PinholeCamera::getViewport( uint32& width, uint32& height )
{
	width  = _screenWidth;
	height = _screenHeight;
}

void PinholeCamera::getRayOrigin( vr::vec3f& origin, float x, float y )
{
	origin = _position;
}

void PinholeCamera::getRayDirection( vr::vec3f& dir, float x, float y )
{
	const float uStep = x * _invWidth;
	const float vStep = y * _invHeight;

	dir = _baseDir + _nearU*uStep + _nearV*vStep;
}

const vr::vec3f& PinholeCamera::getPosition() const
{
	return _position;
}

const vr::vec3f& PinholeCamera::getBaseDir() const
{
	return _baseDir;
}

const vr::vec3f& PinholeCamera::getNearU() const
{
	return _nearU;
}

const vr::vec3f& PinholeCamera::getNearV() const
{
	return _nearV;
}

char* PinholeCamera::getMemberStartAddress()
{
	return (char*)&_dirty;
}

uint32 PinholeCamera::getMemberSize()
{
	return uint32( (char*)&_baseDir - getMemberStartAddress() + 3 );
}

void PinholeCamera::setDirty( bool dirty )
{
	_dirty = dirty;
}
