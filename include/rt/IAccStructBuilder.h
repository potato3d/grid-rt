#ifndef _RT_IACCSTRUCTBUILDER_H_
#define _RT_IACCSTRUCTBUILDER_H_

#include <rt/common.h>
#include <rt/Geometry.h>
#include <rt/Instance.h>
#include <rt/IPlugin.h>
#include <rt/IAccStruct.h>

namespace rt {

class IAccStructBuilder : public IPlugin
{
public:
	// Default implementation: do nothing
	virtual void buildGeometry( rt::Geometry* geometry );
	virtual IAccStruct* buildInstance( const std::vector<rt::Instance> instances );
};

} // namespace rt

#endif // _RT_IACCSTRUCTBUILDER_H_
