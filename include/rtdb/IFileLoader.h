#ifndef _RTDB_IFILELOADER_H_
#define _RTDB_IFILELOADER_H_

#include <rtdb/rtdb.h>
#include <vr/ref_counting.h>

namespace rtdb {

class IFileLoader : public vr::RefCounted
{
public:
	virtual void registerSupportedExtensions() = 0;
	virtual bool loadGeometry( const vr::String& filename ) = 0;

protected:
	~IFileLoader()
	{
		// empty
	}
};

} // namespace rtdb

#endif // _RTDB_IFILELOADER_H_
