#ifndef _RTGL_GPURENDERER_H_
#define _RTGL_GPURENDERER_H_

#include <rtgl/common.h>
#include <rtgl/ShaderProgram.h>
#include <rtgl/FrameBufferObject.h>
#include <rt/IRenderer.h>
#include <rt/Sphere.h>
#include <vr/vec3.h>
#include <vr/timer.h>

#include <GpuGrid.h>

#include <rtgl/GpuObjLoader.h>

namespace rtgl {

class GpuRenderer : public rt::IRenderer
{
public:
	GpuRenderer();

	void setAnimationEnabled( bool enabled );

	virtual void newFrame();
	virtual void render();

	void update();

	void reloadShaders();

	void rebuildGrid( bool recomputeResolution );

	void uploadKeyframe( int frameId, int texId );

	ObjAnimation objAnimation;
	GpuGrid _grid;

private:
	ShaderProgram _shaders;
	ShaderProgram _shaders2;
	ShaderProgram _shaders3;
	vr::vec3f _lowerLeftRayDir;
	vr::vec3f _lowerRightRayDir;
	vr::vec3f _upperRightRayDir;
	vr::vec3f _upperLeftRayDir;

	GLuint _auxFBO;

	int _currFrameId;

	int _currBufferId;

	bool _needRebuild;

	std::vector<GLuint> _triangleVerticesPBO;
	std::vector<GLuint> _triangleNormalsPBO;
	std::vector<GLuint> _triangleTexCoordsPBO;
	std::vector<GLuint> _materialsPBO;

	std::vector< vr::ref_ptr<rtgl::Texture> > _triangleVertices;
	std::vector< vr::ref_ptr<rtgl::Texture> > _triangleNormals;
	std::vector< vr::ref_ptr<rtgl::Texture> > _triangleTexCoords;
	std::vector< vr::ref_ptr<rtgl::Texture> > _materials;
	uint32 _materialTextureArray;

	vr::Timer _frameTimer;

	bool _animationEnabled;

};

} // namespace rtgl

#endif // _RTGL_GPURENDERER_H_
