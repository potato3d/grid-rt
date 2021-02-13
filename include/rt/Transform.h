#ifndef _RT_TRANSFORM_H_
#define _RT_TRANSFORM_H_

#include <rt/common.h>
#include <rt/Ray.h>
#include <vr/mat4.h>

namespace rt {

class Transform
{
public:
	Transform();

	void setMatrix( const vr::mat4f& matrix );
	inline const vr::mat4f& getMatrix() const;

	void transformVertex( vr::vec3f& vertex ) const;
	void transformNormal( vr::vec3f& normal ) const;

	inline void inverseTransform( Ray& ray ) const;

private:
	vr::mat4f _matrix;
	vr::mat4f _inverseMatrix;
	// (M^T)^-1
	// Inverse of transposed matrix, first transpose then invert!
	vr::mat4f _inverseOfTransposedMatrix;
};

inline void Transform::inverseTransform( Ray& ray ) const
{
	// Transform ray origin
	_inverseMatrix.transform( ray.orig );

	// Transform ray direction without normalizing
	// Ignore translation part of matrix
	// Direction has w = 0 in homogeneous coordinates
	_inverseMatrix.transform3x3( ray.dir );
}

inline const vr::mat4f& Transform::getMatrix() const
{
	return _matrix;
}

} // namespace rt

#endif // _RT_TRANSFORM_H_
