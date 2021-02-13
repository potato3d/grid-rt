#pragma once
#ifndef _ENVAPP_WDGTRANSFORMEDIT_H_
#define _ENVAPP_WDGTRANSFORMEDIT_H_

#include "ui_WdgTransformEdit.h"
#include <rtgl/GpuRenderer.h>

#include <vr/vec4.h>
#include <sig/sigslot.h>

class WdgTransformEdit : public QWidget, public sig::has_slots<>
{
	Q_OBJECT

public:
	WdgTransformEdit();

	void setGpuRenderer( rtgl::GpuRenderer* gpur );
	void syncFields();

private slots:
	/// Spin box fields
	void on_sbX_valueChanged( int value );
	void on_sbY_valueChanged( int value );
	void on_sbZ_valueChanged( int value );
	void on_dsbK_valueChanged( double value );

	/// Toggle absolute/relative transform
	void on_pbApply_clicked();

private:
	Ui::WdgTransformEdit _ui;
	bool _disableGuiEvents;
	rtgl::GpuRenderer* _gpur;
};

#endif // _ENVAPP_WDGTRANSFORMEDIT_H_
