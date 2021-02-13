#ifndef _RT_ILIGHT_H_
#define _RT_ILIGHT_H_

#include <rt/IPlugin.h>
#include <rt/Sample.h>

namespace rt {

class ILight : public IPlugin
{
public:
	// Default implementation: do nothing
	virtual bool illuminate( rt::Sample& sample );
};

} // namespace rt

#endif // _RT_ILIGHT_H_
