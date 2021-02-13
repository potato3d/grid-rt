#ifndef _RTGL_COMMON_H_
#define _RTGL_COMMON_H_

#include <gl/glew.h>
#include <rt/common.h>

namespace rtgl {

static GLenum initContext()
{
	return glewInit();
}

}

#endif // _RTGL_COMMON_H_
