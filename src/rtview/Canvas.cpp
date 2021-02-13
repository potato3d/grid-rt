#include <gl/glew.h>
#include "Canvas.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>

// Main rt class
#include <rt/Context.h>
#include <rt/Scene.h>

// Needed for camera manipulation
#include <rt/ICamera.h>
#include <rtp/SimpleEnvironment.h>

// File handling
#include <rtdb/FileManager.h>
#include <rtdb/TriMeshLoader.h>
#include <rtdb/ObjFileLoader.h>

#include "QtImageLoader.h"

// Materials
#include <rtp/HeadlightMaterial.h>
#include <rtp/HeadlightMaterialColor.h>
#include <rtp/PhongMaterialColor.h>

// Lights
#include <rtp/SimplePointLight.h>

// Renderers
#include <rtp/SingleThreadRenderer.h>
#include <rtp/MultiThreadRenderer.h>
#include <rtp/TiledRenderer.h>
#include <rtc/CudaRenderer.h>

// Acceleration structures
#include <rtp/UniformGridAccStructBuilder.h>
#include <rtp/KdTreeAccStructBuilder.h>

Canvas::Canvas( QWidget* parent )
: QGLWidget( createDefaultGLFormat(), parent )
{
	_redrawPolicy = Redraw_AsNeeded;
	_cameraMoveSpeed = 0.05f;
	_cameraRotateSpeed = 0.001f;
	setFocusPolicy( Qt::StrongFocus );
	_cameraFilename = "DefaultCamera.bin";
	_currentPath = "../data/utah";
	_gpur = new rtgl::GpuRenderer();
}

void Canvas::loadDefaultScene()
{
	rt::Context* ctx = rt::Context::current();

	// Default material
	ctx->bindMaterial( _defaultMaterialId );

	// Create scene geometry
	uint32 geometryId = ctx->createGeometries( 1 );
	ctx->beginGeometry( geometryId );

	//rtdb::loadTriangle();
	//rtdb::loadEvenSpaced();
	//rtdb::loadLogo();
	//rtdb::loadCube();
	//rtdb::loadTeapot();
	rtdb::loadMixed();
	//rtdb::loadTeapots125();

	ctx->endGeometry();

	// Instantiate geometry
	uint32 instId = ctx->createInstances( 1 );
	ctx->instantiateLastGeometry( instId );

	printCurrentGeometryStats();
	updateCameraFromScene();
}

void Canvas::loadGeometryFile()
{
	QStringList files = QFileDialog::getOpenFileNames( this, tr("Choose one or more geometry files"),
		_currentPath,
		tr( rtdb::FileManager::instance()->getFileFilters().data() ) );

	if( files.isEmpty() )
		return;

	_currentPath = QFileInfo( files[0] ).path();

	// TODO: 
	/**/
	vr::StringList filenames;
	for( unsigned int i  = 0; i < files.size(); ++i )
	{
		filenames.push_back( files[i].toAscii().data() );
	}

	_gpur->objAnimation.loadAnimation( filenames, new QtImageLoader() );

	_gpur->objAnimation.printStats();

	_gpur->update();
	/**
	// TODO: 

	// Load files
	bool anyOk = false;

	for( int i = 0; i < files.size(); ++i )
	{
		bool ok = rtdb::FileManager::instance()->loadGeometry( vr::String( files[i].toAscii() ) );
		if( !ok )
		{
			QMessageBox::critical( this, "Error loading file", 
				                   QString( "Could not load geometry file " ) + '\'' + files[i] + '\'' + "." );
		}

		anyOk |= ok;
	}

	if( !anyOk )
		return;

	rt::Context* ctx = rt::Context::current();

	for( int i = 0; i < ctx->getGeometryCount(); ++i )
		printf( "num triangles: %d\n", ctx->getGeometry( i )->triDesc.size() );

	// Instantiate geometry
	uint32 instanceId = ctx->createInstances( 1 );
	ctx->instantiateLastGeometry( instanceId );

	printCurrentGeometryStats();

	printf( "num textures: %d\n", ctx->getTextureCount() - 1 );

	*/

	updateCameraFromScene();
}

void Canvas::setRedrawPolicy( RedrawPolicy policy )
{
	_redrawPolicy = policy;
	if( policy == Redraw_Always )
		_fpsTimerId = startTimer( 0 );
	else
		killTimer( _fpsTimerId );
}

void Canvas::setRenderMode( RenderMode mode )
{
	_renderMode = mode;
	switch( mode )
	{
	case Render_Cpu_Single:
		rt::Context::current()->setRenderer( new rtp::SingleThreadRenderer() );
		break;

	case Render_Cpu_Multi:
		rt::Context::current()->setRenderer( new rtp::MultiThreadRenderer() );
		break;

	case Render_Gpu_Glsl:
		{
			rt::Context::current()->setRenderer( _gpur.get() );
			break;
		}

	case Render_Gpu_Cuda:
		{
			rtc::CudaRenderer* cudar = new rtc::CudaRenderer();
			cudar->initialize();
			rt::Context::current()->setRenderer( cudar );
			break;
		}

	default:
	    break;
	}
	updateGL();
}

void Canvas::reloadShaders()
{
	rtgl::GpuRenderer* gpur = dynamic_cast<rtgl::GpuRenderer*>( rt::Context::current()->getRenderer() );
	if( gpur != NULL )
		gpur->reloadShaders();
	updateGL();
}

void Canvas::loadCamera()
{
	QString filename = QFileDialog::getOpenFileName( this, "Load camera file", "./", "Camera files(*.bin)" );
	if( filename.isEmpty() )
		return;

	rtp::PinholeCamera* cam = dynamic_cast<rtp::PinholeCamera*>( rt::Context::current()->getCamera() );

	QFile file( filename );

	if( !file.exists() )
	{
		QMessageBox::warning( this, "Error loading camera file", "File \'" + filename + "\' not found." );
		return;
	}

	file.open( QIODevice::ReadOnly );

	if( !file.isOpen() )
	{
		QMessageBox::warning( this, "Error saving camera file", "Could not open file \'" + filename + "\' for reading." );
		return;
	}

	file.read( cam->getMemberStartAddress(), cam->getMemberSize() );
	file.close();

	// Force camera update
	cam->setDirty( true );
}

void Canvas::saveCamera()
{
	QString filename = QFileDialog::getSaveFileName( this, "Save camera file", "./", "Camera files (*.bin)" );
	if( filename.isEmpty() )
		return;

	rtp::PinholeCamera* cam = dynamic_cast<rtp::PinholeCamera*>( rt::Context::current()->getCamera() );

	QFile file( filename );

	file.open( QIODevice::WriteOnly );

	if( !file.isOpen() )
	{
		QMessageBox::warning( this, "Error saving camera file", "Could not open file \'" + filename + "\' for writing." );
		return;
	}

	file.write( cam->getMemberStartAddress(), cam->getMemberSize() );
	file.close();
}

rtgl::GpuRenderer* Canvas::getGpuRenderer()
{
	return _gpur.get();
}

/************************************************************************/
/* Protected                                                            */
/************************************************************************/
void Canvas::timerEvent( QTimerEvent* e )
{
	updateGL();
}

void Canvas::initializeGL()
{
	// Needed to initialize qt's current glcontext properly
	renderText( 0, 0, "HELLO" );

	// Init OpenGL, CUDA and Ray Tracing contexts
	initContexts();
	rt::Context* ctx = rt::Context::current();

	// Set environment
	ctx->setEnvironment( new rtp::SimpleEnvironment() );

	// Testing acc structs
	ctx->setAccStructBuilder( new rtp::UniformGridAccStructBuilder() );
	//ctx->setAccStructBuilder( new rtp::KdTreeAccStructBuilder() );

	// Setup default material
	rtp::HeadlightMaterialColor* mat = new rtp::HeadlightMaterialColor;
	mat->setDiffuse( 0.0f, 0.0f, 1.0f );

	_defaultMaterialId = ctx->createMaterials( 1 );
	ctx->setMaterial( _defaultMaterialId, mat );

	// Add lights
	uint32 lid = ctx->createLights( 1 );
	rtp::SimplePointLight* light = new rtp::SimplePointLight();
	light->setPosition( 1000.0f, 1000.0f, 1000.0f );
	ctx->setLight( lid, light );

	// Setup default camera
	ctx->setCamera( new rtp::PinholeCamera() );

	// PBO setup
	_pbo = rtgl::PixelBufferObject::create();
	bool ok = _pbo->init();
	if( !ok )
	{
		QMessageBox::critical( this, "Error initializing context", 
			"Could not initialize Pixel Buffer Object properly, press OK to exit." );
		exit( 1 );
	}

	// Setup texture for PBO
	glEnable( GL_TEXTURE_RECTANGLE_ARB );
	rtgl::Texture* frameBuffer = rtgl::Texture::create();
	frameBuffer->init( rtgl::Texture::Format_RGB_Ubyte, rtgl::Texture::Format_RGB_Float, rtgl::Texture::Target_2D_Unormalized );
	frameBuffer->setImage( NULL, width(), height() );
	_pbo->setTexture( frameBuffer );

	// Set startup renderer, cannot call setRenderMode() because it uses updateGL that will call this again!
	rt::Context::current()->setRenderer( _gpur.get() );
	_renderMode = Render_Gpu_Glsl;

	// OpenGL projection setup to render full screen quad
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, 1, 0, 1, -1, 1 );
}

void Canvas::resizeGL( int w, int h )
{
	glViewport( 0, 0, w, h );
	rt::Context::current()->getCamera()->setViewport( w, h );
	_pbo->reallocate( GL_STREAM_DRAW, w, h );
}

void Canvas::paintGL()
{
	glViewport( 0, 0, width(), height() );

	++_frameCounter;

	if( _renderMode == Render_Gpu_Glsl || _renderMode == Render_Gpu_Cuda )
	{
		// No need to use PBO
		rt::Context::current()->renderFrame();
	}
	else
	{
		// Bind frame buffer texture and PBO
		glActiveTexture( GL_TEXTURE0 );
		_pbo->bind();

		// Map PBO to a virtual pointer
		void* deviceMem = _pbo->beginWrite();
		rt::Context::current()->setFrameBuffer( static_cast<float*>( deviceMem ) );

		// Do actual ray tracing of current frame
		rt::Context::current()->renderFrame();

		// Transfer updated PBO to GPU
		_pbo->endWrite();

		// Do actual display of current frame
		glColor3f( 1, 0, 0 );
		glBegin( GL_QUADS );
			glTexCoord2f( 0, 0 );
			glVertex2f( 0, 0 );

			glTexCoord2f( width(), 0 );
			glVertex2f( 1, 0 );

			glTexCoord2f( width(), height() );
			glVertex2f( 1, 1 );

			glTexCoord2f( 0, height() );
			glVertex2f( 0, 1 );
		glEnd();

		// Deactivate texture and PBO
		_pbo->release();
	}

	// Update fps timer
	double elapsed = _timer.elapsed();
	if( ( _redrawPolicy == Redraw_Always ) && ( elapsed >= 1.0 ) )
	{
		emit updateFps( _frameCounter / elapsed );
		_frameCounter = 0;
		_timer.restart();
	}
}

void Canvas::keyPressEvent( QKeyEvent* e )
{
	if( e->key() == Qt::Key_Space )
		updateCameraFromScene();

	if( e->isAutoRepeat() )
		return;

	// Move camera with keyboard
	rt::ICamera* camera = rt::Context::current()->getCamera();

	QString keys = e->text();

	if( keys.contains( 'w', Qt::CaseInsensitive ) )
		camera->continuousTranslation.z += -_cameraMoveSpeed;

	if( keys.contains( 'a', Qt::CaseInsensitive ) )
		camera->continuousTranslation.x += -_cameraMoveSpeed;

	if( keys.contains( 's', Qt::CaseInsensitive ) )
		camera->continuousTranslation.z += _cameraMoveSpeed;

	if( keys.contains( 'd', Qt::CaseInsensitive ) )
		camera->continuousTranslation.x += _cameraMoveSpeed;

	if( keys.contains( 'r', Qt::CaseInsensitive ) )
		camera->continuousTranslation.y += _cameraMoveSpeed;

	if( keys.contains( 'f', Qt::CaseInsensitive ) )
		camera->continuousTranslation.y += -_cameraMoveSpeed;

	e->accept();
	updateGL();
}

void Canvas::keyReleaseEvent( QKeyEvent* e )
{
	if( e->isAutoRepeat() )
		return;

	// Move camera with keyboard
	rt::ICamera* camera = rt::Context::current()->getCamera();

	QString keys = e->text();

	if( keys.contains( 'w', Qt::CaseInsensitive ) )
		camera->continuousTranslation.z -= -_cameraMoveSpeed;

	if( keys.contains( 'a', Qt::CaseInsensitive ) )
		camera->continuousTranslation.x -= -_cameraMoveSpeed;

	if( keys.contains( 's', Qt::CaseInsensitive ) )
		camera->continuousTranslation.z -= _cameraMoveSpeed;

	if( keys.contains( 'd', Qt::CaseInsensitive ) )
		camera->continuousTranslation.x -= _cameraMoveSpeed;

	if( keys.contains( 'r', Qt::CaseInsensitive ) )
		camera->continuousTranslation.y -= _cameraMoveSpeed;

	if( keys.contains( 'f', Qt::CaseInsensitive ) )
		camera->continuousTranslation.y -= -_cameraMoveSpeed;

	e->accept();
	updateGL();
}


void Canvas::mousePressEvent( QMouseEvent* e )
{
	_previousMousePos = e->globalPos();
	e->accept();
	updateGL();
}

void Canvas::mouseMoveEvent( QMouseEvent* e )
{
	int dx = e->globalPos().x() - _previousMousePos.x();
	int dy = e->globalPos().y() - _previousMousePos.y();
	Qt::MouseButtons buttonMask = e->buttons();

	if( buttonMask == Qt::NoButton )
	{
		e->ignore();
		return;
	}

	rt::ICamera* camera = rt::Context::current()->getCamera();

	if( buttonMask & Qt::LeftButton )
	{
		// Rotate in Y axis
		float angle = -dx * _cameraRotateSpeed;
		camera->rotate( angle, RT_AXIS_Y );

		// Rotate in X axis
		angle = -dy * _cameraRotateSpeed;
		camera->rotate( angle, RT_AXIS_X );
	}
	if( buttonMask & Qt::RightButton )
	{
		// Translate in X axis
		camera->translate( dx * _cameraMoveSpeed, 0.0f, 0.0f );

		// Translate in Y axis
		camera->translate( 0.0f, -dy * _cameraMoveSpeed, 0.0f );
	}
	if( buttonMask & Qt::MidButton )
	{
		// Translate in Z axis
		camera->translate( 0.0f, 0.0f, -dy * _cameraMoveSpeed );
	}
	
	// Update state
	_previousMousePos = e->globalPos();
	e->accept();
	updateGL();
}

void Canvas::wheelEvent( QWheelEvent* e )
{
	rt::ICamera* camera = rt::Context::current()->getCamera();
	camera->translate( 0.0f, 0.0f, -e->delta() * _cameraMoveSpeed * 0.5f );
	e->accept();
	updateGL();
}

//////////////////////////////////////////////////////////////////////////
// Private
//////////////////////////////////////////////////////////////////////////
QGLFormat Canvas::createDefaultGLFormat() const
{
	QGLFormat format;

	// Disable V-Sync
	format.setSwapInterval( 0 );

	return format;
}

void Canvas::initContexts()
{
	//////////////////////////////////////////////////////////////////////////
	// OpenGL
	GLenum glCode = rtgl::initContext();
	if( glCode != GL_NO_ERROR )
	{
		QMessageBox::critical( this, "Error initializing context", 
			"Could not initialize OpenGL context properly, press OK to exit." );
		exit( 1 );
	}

	//////////////////////////////////////////////////////////////////////////
	// CUDA
	bool cudaOk = rtc::initContext();
	if( !cudaOk )
	{
		QMessageBox::critical( this, "Error initializing context", 
			"Could not initialize CUDA context properly, press OK to exit." );
		exit( 1 );
	}

	//////////////////////////////////////////////////////////////////////////
	// rtdb
	rtdb::FileManager::instance()->addFileLoader( new rtdb::TriMeshLoader() );
	rtdb::ObjFileLoader* objLoader = new rtdb::ObjFileLoader();
	objLoader->setImageLoader( new QtImageLoader() );
	rtdb::FileManager::instance()->addFileLoader( objLoader );

	//////////////////////////////////////////////////////////////////////////
	// rtcore
	RTenum rtCode = rt::Context::createNew();
	if( rtCode != RT_OK )
	{
		QMessageBox::critical( this, "Error initializing context", 
			"Could not initialize Ray Tracing context properly, press OK to exit." );
		exit( 1 );
	}

	rt::Context::makeCurrent( rt::Context::getNumActiveContexts() - 1 );
}

void Canvas::updateCameraFromScene()
{
	//rt::Context* ctx = rt::Context::current();

	//ctx->checkAndUpdateInstances();
	//const rt::Aabb& sceneBox = ctx->getScene()->accStruct->getBoundingBox();
	const rt::Aabb& sceneBox = _gpur->objAnimation.bbox;

	vr::vec3f center = ( sceneBox.maxv + sceneBox.minv ) * 0.5f;
	vr::vec3f extents = sceneBox.maxv - sceneBox.minv;
	_cameraMoveSpeed = ( extents.x + extents.y + extents.z ) * ( 0.005f / 3.0f );

	float radius = 1.2f * vr::max( extents.x, vr::max( extents.y, extents.z ) );

	rt::ICamera* cam = rt::Context::current()->getCamera();
	cam->setLookAt( center.x, center.y, center.z + radius,
		            center.x, center.y, center.z, 
					0.0f, 1.0f, 0.0f );

	updateGL();
}

void Canvas::printCurrentGeometryStats()
{
	rt::Context* ctx = rt::Context::current();
	rt::Geometry* geom = ctx->getGeometry( ctx->getGeometryCount() - 1 );
	printf( "\n***** Geometry *****\n" );
	printf( "triangles:  %6d\n", geom->triDesc.size() );
	printf( "vertices:   %6d\n", geom->vertices.size() );
	printf( "normals:    %6d\n", geom->normals.size() );
	printf( "colors:     %6d\n", geom->colors.size() );
	printf( "texCoords:  %6d\n", geom->texCoords.size() );
	printf( "expanded:   %6d\n\n", geom->triDesc.size() * 3 );
}
