#include "DlgCanvasSize.h"

DlgCanvasSize::DlgCanvasSize(QWidget *parent, Qt::WFlags flags)
: QDialog( parent, flags )
{
	ui.setupUi( this );
	_mainWindow = NULL;
	_canvas = NULL;
}

DlgCanvasSize::~DlgCanvasSize()
{
	// empty
}

void DlgCanvasSize::setMainWindow( QWidget* window )
{
	_mainWindow = window;
}

void DlgCanvasSize::setCanvas( QWidget* canvas )
{
	_canvas = canvas;
}

void DlgCanvasSize::on_leCanvasWidth_editingFinished()
{
	double w = ui.leCanvasWidth->text().toDouble();
	double diff = w - _canvas->width();

	_mainWindow->resize( _mainWindow->width() + diff, _mainWindow->height() );
}

void DlgCanvasSize::on_leCanvasHeight_editingFinished()
{
	double h = ui.leCanvasHeight->text().toDouble();
	double diff = h - _canvas->height();

	_mainWindow->resize( _mainWindow->width(), _mainWindow->height() + diff );
}

void DlgCanvasSize::showEvent( QShowEvent* event )
{
	ui.leCanvasWidth->setText( QString::number( _canvas->width() ) );
	ui.leCanvasHeight->setText( QString::number( _canvas->height() ) );
}
