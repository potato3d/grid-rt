#ifndef _RTDB_OBJFILELOADER_H_
#define _RTDB_OBJFILELOADER_H_

#include <rtdb/rtdb.h>
#include <rtdb/IFileLoader.h>

namespace rtdb {

class IImageLoader
{
public:
	virtual bool loadImage( const char* filename ) = 0;
	virtual uint32 getWidth() = 0;
	virtual uint32 getHeight() = 0;
	virtual unsigned char* getImage() = 0;
};

class ObjFileLoader : public IFileLoader
{
public:
	ObjFileLoader();

	virtual void registerSupportedExtensions();
	virtual bool loadGeometry( const vr::String& filename );

	void setImageLoader( IImageLoader* loader );

private:
	IImageLoader* _imgLoader;
};

} // namespace rtdb

#endif // _RTDB_OBJFILELOADER_H_
