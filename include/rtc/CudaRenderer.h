#ifndef _RTC_CUDARENDERER_H_
#define _RTC_CUDARENDERER_H_

#include <rtc/common.h>
#include <rt/IRenderer.h>
#include <rtgl/PixelBufferObject.h>

namespace rtc {

class CudaRenderer : public rt::IRenderer
{
public:
	virtual void newFrame();
	virtual void render();

	void initialize();

private:
	void updateCudaFrameBuffer( uint32 width, uint32 height );

	vr::ref_ptr<rtgl::PixelBufferObject> _frameBuffer;
};

} // namespace rtc

#endif // _RTC_CUDARENDERER_H_
