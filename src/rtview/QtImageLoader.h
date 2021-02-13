#ifndef PPMFILELOADER_H_
#define PPMFILELOADER_H_

#include <rtdb/ObjFileLoader.h>
#include <QImage.h>

class QtImageLoader : public rtdb::IImageLoader
{
public:
	virtual bool loadImage( const char* filename );
	virtual uint32 getWidth();
	virtual uint32 getHeight();
	virtual unsigned char* getImage();

private:
	QImage _image;
};

#endif // PPMFILELOADER_H_
