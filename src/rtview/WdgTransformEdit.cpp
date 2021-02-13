#include "WdgTransformEdit.h"
#include <GpuGrid.h>

WdgTransformEdit::WdgTransformEdit()
{
	_ui.setupUi( this );
	_disableGuiEvents = false;
	_gpur = NULL;
}

void WdgTransformEdit::setGpuRenderer( rtgl::GpuRenderer* gpur )
{
	_gpur = gpur;
}
void WdgTransformEdit::syncFields()
{
	_disableGuiEvents = true;
	_ui.sbX->setValue( _gpur->_grid._numCells.x );
	_ui.sbY->setValue( _gpur->_grid._numCells.y );
	_ui.sbZ->setValue( _gpur->_grid._numCells.z );
	_ui.dsbK->setValue( _gpur->_grid._k );
	_disableGuiEvents = false;
}

void WdgTransformEdit::on_sbX_valueChanged( int value )
{
	if( _disableGuiEvents )
		return;
}

void WdgTransformEdit::on_sbY_valueChanged( int value )
{
	if( _disableGuiEvents )
		return;
}

void WdgTransformEdit::on_sbZ_valueChanged( int value )
{
	if( _disableGuiEvents )
		return;
}

void WdgTransformEdit::on_dsbK_valueChanged( double value )
{
	if( _disableGuiEvents )
		return;
}

void WdgTransformEdit::on_pbApply_clicked()
{
	bool recomputeResolution = false;

	if( _gpur->_grid._k != _ui.dsbK->value() )
		recomputeResolution = true;

	_gpur->_grid._numCells.x = _ui.sbX->value();
	_gpur->_grid._numCells.y = _ui.sbY->value();
	_gpur->_grid._numCells.z = _ui.sbZ->value();
	_gpur->_grid.updateCellSize();

	_gpur->_grid._k = _ui.dsbK->value();

	// re-create grid
	_gpur->rebuildGrid( recomputeResolution );

	_gpur->reloadShaders();

	syncFields();
}
