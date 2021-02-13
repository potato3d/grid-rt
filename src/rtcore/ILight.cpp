#include <rt/ILight.h>

using namespace rt;

bool ILight::illuminate( rt::Sample& sample )
{
	// avoid warnings
	sample;
	return false;
}
