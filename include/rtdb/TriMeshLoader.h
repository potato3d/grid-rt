#ifndef _RTDB_TRIMESHLOADER_H_
#define _RTDB_TRIMESHLOADER_H_

#include <rtdb/rtdb.h>
#include <rtdb/IFileLoader.h>

namespace rtdb {

class TriMeshLoader : public IFileLoader
{
public:
	virtual void registerSupportedExtensions();
	virtual bool loadGeometry( const vr::String& filename );
};

} // namespace rtdb

#endif // _RTDB_TRIMESHLOADER_H_
