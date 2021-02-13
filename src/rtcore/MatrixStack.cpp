#include <rt/MatrixStack.h>

using namespace rt;

MatrixStack::MatrixStack()
{
	_stack.push( vr::mat4f() );
	loadIdentity();
}

const vr::mat4f& MatrixStack::top()
{
	return _stack.top();
}

void MatrixStack::pushMatrix()
{
	_stack.push( _stack.top() );
}

void MatrixStack::loadIdentity()
{
	_stack.top().makeIdentity();
}

void MatrixStack::scale( float x, float y, float z )
{
	vr::mat4f m;
	m.makeScale( x, y, z );
	_stack.top().product( _stack.top(), m );
}

void MatrixStack::translate( float x, float y, float z )
{
	vr::mat4f m;
	m.makeTranslation( x, y, z );
	_stack.top().product( _stack.top(), m );
}

void MatrixStack::rotate( float degrees, float x, float y, float z )
{
	vr::mat4f m;
	m.makeRotation( vr::toRadians( degrees ), x, y, z );
	_stack.top().product( _stack.top(), m );
}

void MatrixStack::loadMatrix( const float* const matrix )
{
	_stack.top().set( matrix );
}

void MatrixStack::multMatrix( const float* const matrix )
{
	_stack.top().product( _stack.top(), vr::mat4f( matrix ) );
}

void MatrixStack::popMatrix()
{
	if( _stack.size() <= 1 )
		return;

	_stack.pop();
}
