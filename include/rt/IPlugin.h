#ifndef _RT_IPLUGIN_H_
#define _RT_IPLUGIN_H_

#include <rt/common.h>
#include <vr/ref_counting.h>

namespace rt {

class IPlugin : public vr::RefCounted
{
public:
	// Default implementation: do nothing
	virtual void newFrame();
	virtual void receiveParameter( int paramId, void* paramValue );

protected:
	virtual ~IPlugin();
};

} // namespace rt

#endif // _RT_IPLUGIN_H_
