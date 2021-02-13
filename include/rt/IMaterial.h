#ifndef _RT_IMATERIAL_H_
#define _RT_IMATERIAL_H_

#include <rt/IPlugin.h>
#include <rt/Sample.h>

namespace rt {

class IMaterial : public IPlugin
{
public:
	// Default implementation: do nothing
	virtual void shade( rt::Sample& sample );
};

} // namespace rt

#endif // _RT_IMATERIAL_H_
