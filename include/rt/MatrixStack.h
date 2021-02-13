#ifndef _RT_MATRIXSTACK_H_
#define _RT_MATRIXSTACK_H_

#include <rt/common.h>
#include <vr/mat4.h>
#include <stack>

namespace rt {

class MatrixStack
{
public:
	MatrixStack();

	const vr::mat4f& top();

	void pushMatrix();
	void loadIdentity();
	void scale( float x, float y, float z );
	void translate( float x, float y, float z );
	void rotate( float degrees, float x, float y, float z );
	void loadMatrix( const float* const matrix );
	void multMatrix( const float* const matrix );
	void popMatrix();

private:
	std::stack<vr::mat4f> _stack;
};

} // namespace rt

#endif // _RT_MATRIXSTACK_H_
