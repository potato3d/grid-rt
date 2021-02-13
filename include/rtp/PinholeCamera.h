#ifndef _RTP_PINHOLECAMERA_H_
#define _RTP_PINHOLECAMERA_H_

#include <rt/ICamera.h>
#include <vr/quat.h>

namespace rtp {

class PinholeCamera : public rt::ICamera
{
public:
	PinholeCamera();

	virtual void newFrame();

	virtual void translate( float x, float y, float z );
	virtual void rotate( float radians, RTenum axis );

	virtual void setLookAt( float eyeX, float eyeY, float eyeZ, 
		                    float centerX, float centerY, float centerZ, 
		                    float upX, float upY, float upZ );

	virtual void setPerspective( float fovY, float zNear, float zFar );

	virtual void setViewport( uint32 width, uint32 height );
	virtual void getViewport( uint32& width, uint32& height );

	virtual void getRayOrigin( vr::vec3f& origin, float x, float y );
	virtual void getRayDirection( vr::vec3f& dir, float x, float y );

	const vr::vec3f& getPosition() const;

	// Base ray direction
	const vr::vec3f& getBaseDir() const;

	// Near U and V vectors to interpolate from base ray direction
	const vr::vec3f& getNearU() const;
	const vr::vec3f& getNearV() const;

	// TODO: do this the proper way!!
	// Helpers for serializing
	char* getMemberStartAddress();
	uint32 getMemberSize();
	void setDirty( bool dirty );

private:
	// Has changed since the last update?
	bool _dirty;

	// Input parameters
	vr::vec3f _position;
	vr::quatf _orientation;

	float _fovy;
	float _zNear;
	float _zFar;

	uint32 _screenWidth;
	uint32 _screenHeight;

	// Derived values
	vr::vec3f _axisX;		// camera axis
	vr::vec3f _axisY;
	vr::vec3f _axisZ;

	vr::vec3f _nearOrigin;	// near plane origin
	vr::vec3f _nearU;		// near plane U vector
	vr::vec3f _nearV;		// near plane V vector

	// Pre-computation
	float _invWidth;
	float _invHeight;
	vr::vec3f _baseDir;
};

} // namespace rtp

#endif // _RTP_PINHOLECAMERA_H_
