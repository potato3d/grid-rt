#pragma once
#ifndef _CANVAS_H_
#define _CANVAS_H_

#include <rtgl/PixelBufferObject.h>
#include <rtp/PinholeCamera.h>
#include <QtOpenGL/QGLWidget>
#include <vr/timer.h>

#include <rtgl/GpuRenderer.h>

class Canvas : public QGLWidget
{
	Q_OBJECT
public:
	enum RedrawPolicy
	{
		Redraw_AsNeeded,
		Redraw_Always
	};

	enum RenderMode
	{
		Render_Cpu_Single,
		Render_Cpu_Multi,
		Render_Gpu_Glsl,
		Render_Gpu_Cuda
	};

	Canvas( QWidget* parent );

	void loadDefaultScene();
	void loadGeometryFile();
	void setRedrawPolicy( RedrawPolicy policy );
	void setRenderMode( RenderMode mode );
	void reloadShaders();

	void loadCamera();
	void saveCamera();

	rtgl::GpuRenderer* getGpuRenderer();

signals:
	void updateFps( double fps );

protected:
	virtual void timerEvent( QTimerEvent* e );

	virtual void initializeGL();
	virtual void resizeGL( int w, int h );
	virtual void paintGL();

	virtual void keyPressEvent( QKeyEvent* e );
	virtual void keyReleaseEvent( QKeyEvent* e );
	virtual void mousePressEvent( QMouseEvent* e );
	virtual void mouseMoveEvent( QMouseEvent* e );
	virtual void wheelEvent( QWheelEvent* e );

private:
	QGLFormat createDefaultGLFormat() const;
	void initContexts();
	void updateCameraFromScene();
	void printCurrentGeometryStats();

	RedrawPolicy _redrawPolicy;
	RenderMode _renderMode;
	int _fpsTimerId;
	vr::Timer _timer;
	unsigned int _frameCounter;

	unsigned int _defaultMaterialId;

	float _cameraMoveSpeed;
	float _cameraRotateSpeed;
	QPoint _previousMousePos;

	vr::ref_ptr<rtgl::PixelBufferObject> _pbo;
	QString _cameraFilename;
	QString _currentPath;

	vr::ref_ptr<rtgl::GpuRenderer> _gpur;
};

#endif // _CANVAS_H_
