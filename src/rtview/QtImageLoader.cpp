#include "QtImageLoader.h"

bool QtImageLoader::loadImage( const char* filename )
{
	bool ok = _image.load( filename );
	if( !ok )
		return false;

	_image = _image.convertToFormat( QImage::Format_RGB32 );
	_image = _image.rgbSwapped();

	return true;
}

uint32 QtImageLoader::getWidth()
{
	return _image.width();
}

uint32 QtImageLoader::getHeight()
{
	return _image.height();
}

unsigned char* QtImageLoader::getImage()
{
	return _image.bits();
}
