#ifndef _RTP_UNIFORMGRIDACCSTRUCT_H_
#define _RTP_UNIFORMGRIDACCSTRUCT_H_

#include <rt/IAccStruct.h>
#include <rt/Geometry.h>
#include <rt/RayTriIntersection.h>

namespace rtp {

class UniformGridAccStruct : public rt::IAccStruct
{
public:
	typedef std::vector<int32> Cell;

	UniformGridAccStruct();
	~UniformGridAccStruct();

	virtual void clear();
	virtual void traceNearestGeometry( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit );

	// Must have a valid bounding box first!
	void setResolution( int32 nCellsX, int32 nCellsY, int32 nCellsZ );
	void getResolution( int32& nCellsX, int32& nCellsY, int32& nCellsZ ) const;

	const vr::vec3f& getCellSize() const;
	const vr::vec3f& getInvCellSize() const;

	const Cell& at( int32 x, int32 y, int32 z ) const;
	Cell& at( int32 x, int32 y, int32 z );

	int32 worldToVoxel( float value, RTenum axis );
	float voxelToWorld( int32 voxel, RTenum axis );

	const std::vector<Cell>& getGridData() const;

private:
	// Main intersection routine, called whenever we find a non-empty cell
	// Updates ray.tfar to avoid false intersections that lie outside cell boundaries
	bool intersectTriangles( const std::vector<rt::TriAccel>& triangles, const Cell& cell, 
		                     float maxValidDistance, rt::Ray& ray, rt::Hit& hit, float& bestDistance );


	// 3D-DDA traversal
	void traverse3ddda( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit );

	// Cube grid traversal
	void traverseCubeGrid( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit );

	// Number of cells in each dimension
	int32 _nx;
	int32 _ny;
	int32 _nz;

	std::vector<Cell> _data;
	vr::vec3f _cellSize;
	vr::vec3f _invCellSize;
};

} // namespace rtp

#endif // _RTP_UNIFORMGRIDACCSTRUCT_H_
