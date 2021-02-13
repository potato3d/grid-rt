#include <rt/IPlugin.h>

using namespace rt;

void IPlugin::newFrame()
{
	// empty
}

void IPlugin::receiveParameter( int paramId, void* paramValue )
{
	// avoid warnings
	paramId; paramValue;
}

IPlugin::~IPlugin()
{
	// empty
}
