#include <rtp/UniformGridAccStruct.h>
#include <rt/Context.h>
#include <rt/IEnvironment.h>
#include <rt/AabbIntersection.h>
#include <rt/RayTriIntersection.h>

using namespace rtp;

UniformGridAccStruct::UniformGridAccStruct()
{
	_nx = 0;
	_ny = 0;
	_nz = 0;
	_cellSize.set( 0,0,0 );
	_invCellSize.set( 0,0,0 );
}

UniformGridAccStruct::~UniformGridAccStruct()
{
	clear();
}

void UniformGridAccStruct::clear()
{
	vr::vectorFreeMemory( _data );
}

//////////////////////////////////////////////////////////////////////////
// Traversal entry point
void UniformGridAccStruct::traceNearestGeometry( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit )
{
	// If don't hit bbox in local space, no need to trace underlying triangles
	if( !rt::AabbIntersection::clipRay( _bbox, ray ) )
		return;

	traverse3ddda( instance, ray, hit );

	// TODO: 28-2-2008
	// TODO: we tried this grid in order to build inside GPU like Particle Simulation from Waldemar
	// TODO: currently abandoned in favor of traditional grid, because it is at least 2x faster
	//traverseCubeGrid( instance, ray, hit );
}

void UniformGridAccStruct::setResolution( int32 nCellsX, int32 nCellsY, int32 nCellsZ )
{
	// Just in case
	if( _bbox.isDegenerate() )
		return;

	_data.resize( nCellsX*nCellsY*nCellsZ );
	_nx = nCellsX;
	_ny = nCellsY;
	_nz = nCellsZ;

	// Update cell sizes
	_cellSize.set( ( _bbox.maxv.x - _bbox.minv.x ) / (float)_nx, 
		           ( _bbox.maxv.y - _bbox.minv.y ) / (float)_ny,
		           ( _bbox.maxv.z - _bbox.minv.z ) / (float)_nz );

	_invCellSize.x = 1.0f / _cellSize.x;
	_invCellSize.y = 1.0f / _cellSize.y;
	_invCellSize.z = 1.0f / _cellSize.z;
}

void UniformGridAccStruct::getResolution( int32& nCellsX, int32& nCellsY, int32& nCellsZ ) const
{
	nCellsX = _nx;
	nCellsY = _ny;
	nCellsZ = _nz;
}

const vr::vec3f& UniformGridAccStruct::getCellSize() const
{
	return _cellSize;
}

const vr::vec3f& UniformGridAccStruct::getInvCellSize() const
{
	return _invCellSize;
}

const UniformGridAccStruct::Cell& UniformGridAccStruct::at( int32 x, int32 y, int32 z ) const
{
	return _data[x + y * _nx + z * _nx * _ny];
}

UniformGridAccStruct::Cell& UniformGridAccStruct::at( int32 x, int32 y, int32 z )
{
	return _data[x + y * _nx + z * _nx * _ny];
}

int32 UniformGridAccStruct::worldToVoxel( float value, RTenum axis )
{
	// Simulate GPU implementation by forcing all intermediary results to 32-bit precision
	return (float)( (float)( value - _bbox.minv[axis] ) * _invCellSize[axis] );

	//return (int32)( ( value - _bbox.minv[axis] ) * _invCellSize[axis] );
}

float UniformGridAccStruct::voxelToWorld( int32 voxel, RTenum axis )
{
	return (float)voxel * _cellSize[axis] + _bbox.minv[axis];
}

const std::vector<UniformGridAccStruct::Cell>& UniformGridAccStruct::getGridData() const
{
	return _data;
}

//////////////////////////////////////////////////////////////////////////
// Private
//////////////////////////////////////////////////////////////////////////
void UniformGridAccStruct::traverse3ddda( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit )
{
	/************************************************************************/
	/* Initial setup                                                        */
	/************************************************************************/
	// 1. Find initial cell where ray begins

	// Since ray was already clipped against bbox (grid), 
	// ray.tnear gives us the starting t (thus the start point as well)
	const vr::vec3f startPoint = ray.orig + ray.dir * ray.tnear;

	// TODO: check if this is the best solution
	int32 x = vr::clampTo( worldToVoxel( startPoint.x, RT_AXIS_X ), 0, _nx - 1 );
	int32 y = vr::clampTo( worldToVoxel( startPoint.y, RT_AXIS_Y ), 0, _ny - 1 );
	int32 z = vr::clampTo( worldToVoxel( startPoint.z, RT_AXIS_Z ), 0, _nz - 1 );

	// 2. Compute stepX, stepY, stepZ
	const int32 stepX = ray.dirSigns[RT_AXIS_X];
	const int32 stepY = ray.dirSigns[RT_AXIS_Y];
	const int32 stepZ = ray.dirSigns[RT_AXIS_Z];

	// TODO: check if it is faster to use logic operations or to branch based on ray.dir signs for each dimension

	// 3 Compute out of grid limits
	const int32 outX = -ray.dirSignBits[RT_AXIS_X] + _nx * !ray.dirSignBits[RT_AXIS_X];
	const int32 outY = -ray.dirSignBits[RT_AXIS_Y] + _ny * !ray.dirSignBits[RT_AXIS_Y];
	const int32 outZ = -ray.dirSignBits[RT_AXIS_Z] + _nz * !ray.dirSignBits[RT_AXIS_Z];

	// 4. Compute tDeltaX, tDeltaY, tDeltaZ
	const float tDeltaX = vr::abs( _cellSize.x * ray.invDir.x );
	const float tDeltaY = vr::abs( _cellSize.y * ray.invDir.y );
	const float tDeltaZ = vr::abs( _cellSize.z * ray.invDir.z );

	// 5. Compute tNextX, tNextY, tNextZ
	float tMaxX = ( voxelToWorld( x + !ray.dirSignBits[RT_AXIS_X], RT_AXIS_X ) - ray.orig.x ) * ray.invDir.x;
	float tMaxY = ( voxelToWorld( y + !ray.dirSignBits[RT_AXIS_Y], RT_AXIS_Y ) - ray.orig.y ) * ray.invDir.y;
	float tMaxZ = ( voxelToWorld( z + !ray.dirSignBits[RT_AXIS_Z], RT_AXIS_Z ) - ray.orig.z ) * ray.invDir.z;

	/************************************************************************/
	/* Trace ray through grid                                               */
	/************************************************************************/
	const std::vector<rt::TriAccel>& triangles = instance.geometry->triAccel;

	// Best distance considers any previously found hits to prevent false hits in this geometry
	float bestDistance = hit.distance;

	// While inside grid
	do
	{
		// Get current cell
		const Cell& cell = at( x, y, z );

		// If cell contains triangles, test intersection.
		// We send the lesser tMax as the maximum valid distance. This avoids false intersections outside current cell.
		if( !cell.empty() )
		{
			if( intersectTriangles( triangles, cell, vr::min( vr::min( tMaxX, tMaxY ), tMaxZ ), ray, hit, bestDistance ) )
			{
				hit.distance = bestDistance;
				hit.instance = &instance;
				return;
			}
		}

		// Go to next cell, need to decide which dimension is next
		// TODO: could do this without branches? is it worth it?
		if( tMaxX < tMaxY && tMaxX < tMaxZ )
		{
			x += stepX;
			tMaxX += tDeltaX;
		}
		else if( tMaxY < tMaxZ )
		{
			y += stepY;
			tMaxY += tDeltaY;
		}
		else
		{
			z += stepZ;
			tMaxZ += tDeltaZ;
		}

	}  while( x != outX && y != outY && z != outZ );
}

void UniformGridAccStruct::traverseCubeGrid( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit )
{
	/************************************************************************/
	/* Initial setup                                                        */
	/************************************************************************/
	// 1. Find initial cell where ray begins

	// Since ray was already clipped against bbox (grid), 
	// ray.tnear gives us the starting t (thus the start point as well)
	const vr::vec3f startPoint = ray.orig + ray.dir * ray.tnear;

	// TODO: check if this is the best solution
	int32 x = vr::clampTo( worldToVoxel( startPoint.x, RT_AXIS_X ), 0, _nx - 1 );
	int32 y = vr::clampTo( worldToVoxel( startPoint.y, RT_AXIS_Y ), 0, _ny - 1 );
	int32 z = vr::clampTo( worldToVoxel( startPoint.z, RT_AXIS_Z ), 0, _nz - 1 );

	// TODO:
//  	hit.triangleId = 0;
//  	hit.instance = &instance;
//  	hit.v0Coord = (float)x / (float)_nx;
//  	hit.v1Coord = (float)y / (float)_ny;
//  	hit.v2Coord = (float)z / (float)_nz;
//  	return;

	// 2. Compute stepX, stepY, stepZ
	const int32 stepX = ray.dirSigns[RT_AXIS_X];
	const int32 stepY = ray.dirSigns[RT_AXIS_Y];
	const int32 stepZ = ray.dirSigns[RT_AXIS_Z];

	// TODO: check if it is faster to use logic operations or to branch based on ray.dir signs for each dimension

	// 3 Compute out of grid limits
	const int32 outX = -ray.dirSignBits[RT_AXIS_X] + _nx * !ray.dirSignBits[RT_AXIS_X];
	const int32 outY = -ray.dirSignBits[RT_AXIS_Y] + _ny * !ray.dirSignBits[RT_AXIS_Y];
	const int32 outZ = -ray.dirSignBits[RT_AXIS_Z] + _nz * !ray.dirSignBits[RT_AXIS_Z];

	// 4. Compute tDeltaX, tDeltaY, tDeltaZ
	const float tDeltaX = vr::abs( _cellSize.x * ray.invDir.x );
	const float tDeltaY = vr::abs( _cellSize.y * ray.invDir.y );
	const float tDeltaZ = vr::abs( _cellSize.z * ray.invDir.z );

	// 5. Compute tNextX, tNextY, tNextZ
	float tMaxX = ( voxelToWorld( x + !ray.dirSignBits[RT_AXIS_X], RT_AXIS_X ) - ray.orig.x ) * ray.invDir.x;
	float tMaxY = ( voxelToWorld( y + !ray.dirSignBits[RT_AXIS_Y], RT_AXIS_Y ) - ray.orig.y ) * ray.invDir.y;
	float tMaxZ = ( voxelToWorld( z + !ray.dirSignBits[RT_AXIS_Z], RT_AXIS_Z ) - ray.orig.z ) * ray.invDir.z;

	/************************************************************************/
	/* Trace ray through grid                                               */
	/************************************************************************/
	const std::vector<rt::TriAccel>& triangles = instance.geometry->triAccel;

	// Best distance considers any previously found hits to prevent false hits in this geometry
	float bestDistance = hit.distance;
	float maxValidDist;

	// Code to access adjacent cells to be intersected during traversal
	int32 sx = vr::clampAbove( x - 1, 0 );
	int32 ex = vr::clampBelow( x + 1, _nx - 1 );
	int32 sy = vr::clampAbove( y - 1, 0 );
	int32 ey = vr::clampBelow( y + 1, _ny - 1 );
	int32 sz = vr::clampAbove( z - 1, 0 );
	int32 ez = vr::clampBelow( z + 1, _nz - 1 );

	//assert( sx <= ex );
	//assert( sy <= ey );
	//assert( sz <= ez );

	//printf( "\n\n*** ray.dir = %f, %f, %f\n", ray.dir.x, ray.dir.y, ray.dir.z );

	// While inside grid
	do
	{
		//printf( "sx = %d  ex = %d\n", sx, ex );
		//printf( "sy = %d  ey = %d\n", sy, ey );
		//printf( "sz = %d  ez = %d\n", sz, ez );

		// TODO: 
		//maxValidDist = vr::min( vr::min( tMaxX, tMaxY ), tMaxZ );
		maxValidDist = vr::Mathf::MAX_VALUE;

		bool haveHit = false;

		for( int32 k = sz; k <= ez; ++k )
		{
			for( int32 j = sy; j <= ey; ++j )
			{
				for( int32 i = sx; i <= ex; ++i )
				{
					const Cell& cell = at( i, j, k );
					//const Cell& cell = at( x, y, z );

					//printf( "cell: %d, %d, %d\n", i, j, k );
					//printf( "cell: %d, %d, %d\n", x, y, z );

					if( !cell.empty() )
						haveHit |= intersectTriangles( triangles, cell, maxValidDist, ray, hit, bestDistance );
				}
			}
		}

		if( haveHit )
		{
			hit.distance = bestDistance;
			hit.instance = &instance;

			// TODO: grid debug
			//hit.v0Coord = (float)x / (float)_nx;
			//hit.v1Coord = (float)y / (float)_ny;
			//hit.v2Coord = (float)z / (float)_nz;

			return;
		}

		// TODO: 
		//return;

		// Go to next cell, need to decide which dimension is next
		// TODO: could do this without branches? is it worth it?
		if( tMaxX < tMaxY && tMaxX < tMaxZ )
		{
			x += stepX;
			tMaxX += tDeltaX;

			sx = vr::clampTo( x + stepX, 0, _nx - 1 );
			ex = sx;

			sy = vr::clampAbove( y - 1, 0 );
			ey = vr::clampBelow( y + 1, _ny - 1 );

			sz = vr::clampAbove( z - 1, 0 );
			ez = vr::clampBelow( z + 1, _nz - 1 );
		}
		else if( tMaxY < tMaxZ )
		{
			y += stepY;
			tMaxY += tDeltaY;

			sx = vr::clampAbove( x - 1, 0 );
			ex = vr::clampBelow( x + 1, _nx - 1 );

			sy = vr::clampTo( y + stepY, 0, _ny - 1 );
			ey = sy;

			sz = vr::clampAbove( z - 1, 0 );
			ez = vr::clampBelow( z + 1, _nz - 1 );
		}
		else
		{
			z += stepZ;
			tMaxZ += tDeltaZ;

			sx = vr::clampAbove( x - 1, 0 );
			ex = vr::clampBelow( x + 1, _nx - 1 );

			sy = vr::clampAbove( y - 1, 0 );
			ey = vr::clampBelow( y + 1, _ny - 1 );

			sz = vr::clampTo( z + stepZ, 0, _nz - 1 );
			ez = sz;
		}

	} while( x != outX && y != outY && z != outZ );
}

bool UniformGridAccStruct::intersectTriangles( const std::vector<rt::TriAccel>& triangles, 
											   const Cell& cell, float maxValidDistance, 
											   rt::Ray& ray, rt::Hit& hit, float& bestDistance )
{
	// Ray.tfar keeps an upper bound on intersection distances we find
	// This avoids false intersections when the ray actually hits the primitive in another cell outside current one
	ray.tfar = maxValidDistance;

	// Intersect triangles in cell and keep closest hit
	for( int32 t = 0, limit = cell.size(); t < limit; ++t )
	{
		rt::RayTriIntersection::hitWald( triangles[cell[t]], ray, hit, bestDistance );
	}

	// return ( found hit )
	return ( bestDistance < hit.distance );
}
