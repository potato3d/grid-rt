#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QLabel>
#include <QCheckBox>
#include <QActionGroup>
#include "DlgCanvasSize.h"
#include "ui_mainwindow.h"
#include "WdgTransformEdit.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
	~MainWindow();

public slots:
	// File
	void on_actionOpenDefaultScene_triggered();
	void on_actionOpen_triggered();
	void on_actionLoadCamera_triggered();
	void on_actionSaveCamera_triggered();

	// Redraw policy
	void on_actionRedrawAlways_toggled( bool enabled );

	// Shader managing
	void on_actionReloadShaders_triggered();

	// Render modes
	void on_actionSingleCpu_toggled( bool enabled );
	void on_actionMultiCpu_toggled( bool enabled );
	void on_actionGlsl_toggled( bool enabled );
	void on_actionCuda_toggled( bool enabled );

	// View
	void on_actionCanvasSize_triggered();

	void oncbEnableAnimationtoggled( bool enabled );

	void updateFps( double fps );

private:
	Ui::MainWindowClass ui;
	QActionGroup* _actionGroupRenderMode;
	QLabel _fpsLabel;
	DlgCanvasSize* _dlgCanvasSize;
	WdgTransformEdit* _wdg;
	QCheckBox _cbEnableAnimation;
};

#endif // MAINWINDOW_H
