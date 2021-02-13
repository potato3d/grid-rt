#ifndef _RT_IENVIRONMENT_H_
#define _RT_IENVIRONMENT_H_

#include <rt/IPlugin.h>
#include <rt/Sample.h>

namespace rt {

class IEnvironment : public IPlugin
{
public:
	// Default implementation: do nothing
	virtual void shade( rt::Sample& sample );
};

} // namespace rt

#endif // _RT_IENVIRONMENT_H_
