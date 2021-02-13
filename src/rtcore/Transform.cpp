#include <rt/Transform.h>

using namespace rt;

Transform::Transform()
{
	_matrix.makeIdentity();
	_inverseMatrix.makeIdentity();
	_inverseOfTransposedMatrix.makeIdentity();
}

void Transform::setMatrix( const vr::mat4f& matrix )
{
	_matrix = matrix;
	_inverseMatrix = matrix;
	_inverseMatrix.invert();

	// (M^T)^-1
	// Inverse of transposed matrix, first transpose then invert!
	_inverseOfTransposedMatrix = matrix;
	_inverseOfTransposedMatrix.transpose();
	_inverseOfTransposedMatrix.invert();
}

void Transform::transformVertex( vr::vec3f& vertex ) const
{
	_matrix.transform( vertex );
}

void Transform::transformNormal( vr::vec3f& normal ) const
{
	// (M^T)^-1
	// Inverse of transposed matrix, first transpose then invert!
	_inverseOfTransposedMatrix.transform( normal );
}
