#include "mainwindow.h"
#include "DlgCanvasSize.h"
#include <QFileDialog>
#include <QMessageBox>

static DlgCanvasSize* s_dlgCanvasSize = NULL;

MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi( this );

	_fpsLabel.setFixedWidth( 80 );
	_fpsLabel.setAlignment( Qt::AlignRight );
	_fpsLabel.setText( "n/a" );
	ui.statusBar->addPermanentWidget( &_fpsLabel );

	// Render mode action group
	_actionGroupRenderMode = new QActionGroup( this );
	_actionGroupRenderMode->addAction( ui.actionSingleCpu );
	_actionGroupRenderMode->addAction( ui.actionMultiCpu );
	_actionGroupRenderMode->addAction( ui.actionGlsl );
	_actionGroupRenderMode->addAction( ui.actionCuda );
	//ui.actionReloadShaders->setEnabled( false );

	connect( ui.mainCanvas, SIGNAL( updateFps( double ) ), this, SLOT( updateFps( double ) ) );

	// Hide stupid context menu to show/hide main toolbar.
	setContextMenuPolicy( Qt::NoContextMenu );

	_dlgCanvasSize = new DlgCanvasSize;
	_dlgCanvasSize->setMainWindow( this );
	_dlgCanvasSize->setCanvas( centralWidget() );

	_wdg = new WdgTransformEdit();
	_wdg->setGpuRenderer( ui.mainCanvas->getGpuRenderer() );

	ui.mainToolBar->addWidget( _wdg );

	_cbEnableAnimation.setText( "Enable animation" );
	_cbEnableAnimation.setChecked( true );
	connect( &_cbEnableAnimation, SIGNAL( toggled(bool) ), this, SLOT( oncbEnableAnimationtoggled( bool ) ) );
	ui.mainToolBar->addWidget( &_cbEnableAnimation );
}

MainWindow::~MainWindow()
{
	delete _actionGroupRenderMode;
}

/************************************************************************/
/* Slots                                                                */
/************************************************************************/
void MainWindow::on_actionOpenDefaultScene_triggered()
{
	ui.mainCanvas->loadDefaultScene();
	_wdg->syncFields();
}

void MainWindow::on_actionOpen_triggered()
{
	ui.mainCanvas->loadGeometryFile();
	_wdg->syncFields();
}

void MainWindow::on_actionLoadCamera_triggered()
{
	ui.mainCanvas->loadCamera();
	ui.mainCanvas->updateGL();
}

void MainWindow::on_actionSaveCamera_triggered()
{
	ui.mainCanvas->saveCamera();
}

void MainWindow::on_actionRedrawAlways_toggled( bool enabled )
{
	if( enabled )
	{
		ui.mainCanvas->setRedrawPolicy( Canvas::Redraw_Always );
	}
	else
	{
		ui.mainCanvas->setRedrawPolicy( Canvas::Redraw_AsNeeded );
		_fpsLabel.setText( "n/a" );
	}
}

void MainWindow::on_actionReloadShaders_triggered()
{
	ui.mainCanvas->reloadShaders();
}

void MainWindow::on_actionSingleCpu_toggled( bool enabled )
{
	if( enabled )
		ui.mainCanvas->setRenderMode( Canvas::Render_Cpu_Single );
}

void MainWindow::on_actionMultiCpu_toggled( bool enabled )
{
	if( enabled )
		ui.mainCanvas->setRenderMode( Canvas::Render_Cpu_Multi );
}

void MainWindow::on_actionGlsl_toggled( bool enabled )
{
	if( enabled )
	{
		ui.mainCanvas->setRenderMode( Canvas::Render_Gpu_Glsl );
		ui.actionReloadShaders->setEnabled( true );
	}
	else
	{
		ui.actionReloadShaders->setEnabled( false );
	}
}

void MainWindow::on_actionCuda_toggled( bool enabled )
{
	if( enabled )
		ui.mainCanvas->setRenderMode( Canvas::Render_Gpu_Cuda );
}

void MainWindow::on_actionCanvasSize_triggered()
{
	_dlgCanvasSize->show();
}

void MainWindow::oncbEnableAnimationtoggled( bool enabled )
{
	ui.mainCanvas->getGpuRenderer()->setAnimationEnabled( enabled );
}


void MainWindow::updateFps( double fps )
{
	_fpsLabel.setText( QString::number( fps, 'g', 4 ) + " fps" );
}

