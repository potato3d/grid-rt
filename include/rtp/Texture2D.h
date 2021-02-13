#ifndef _RTP_TEXTURE2D_H_
#define _RTP_TEXTURE2D_H_

#include <rt/ITexture.h>

namespace rtp {

class Texture2D : public rt::ITexture
{
public:
	Texture2D();

	void setFilter( RTenum type );
	void setWrapS( RTenum type );
	void setWrapT( RTenum type );
	void setEnvMode( RTenum type );

	// Only supports RGBA format
	virtual void setTextureImage2D( uint32 width, uint32 height, unsigned char* texels );
	virtual void shade( rt::Sample& sample );

//protected:
	RTenum   _filter;
	RTenum   _wrapS;
	RTenum   _wrapT;
	RTenum   _envMode;
	uint32   _width;
	uint32   _height;
	unsigned char* _texels;
};

} // namespace rtp

#endif // _RTP_TEXTURE2D_H_
