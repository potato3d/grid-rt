#ifndef DLGCANVASSIZE_H
#define DLGCANVASSIZE_H

#include <QtGui/QDialog>
#include <QtGui/QShowEvent>
#include "ui_dlgCanvasSize.h"

class DlgCanvasSize : public QDialog
{
	Q_OBJECT

public:
	DlgCanvasSize(QWidget *parent = 0, Qt::WFlags flags = 0);
	~DlgCanvasSize();

	void setMainWindow( QWidget* window );
	void setCanvas( QWidget* canvas );

public slots:
	void on_leCanvasWidth_editingFinished();
	void on_leCanvasHeight_editingFinished();

protected:
	virtual void showEvent( QShowEvent* event );

private:
	Ui::dlgCanvasSize ui;
	QWidget* _mainWindow;
	QWidget* _canvas;
};

#endif // DLGCANVASSIZE_H
