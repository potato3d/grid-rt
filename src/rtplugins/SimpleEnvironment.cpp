#include <rtp/SimpleEnvironment.h>

using namespace rtp;

SimpleEnvironment::SimpleEnvironment()
{
	_background.set( 1, 1, 1 );
}

void SimpleEnvironment::shade( rt::Sample& sample )
{
	sample.color = _background;
}
