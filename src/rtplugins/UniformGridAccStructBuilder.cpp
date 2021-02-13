#include <rtp/UniformGridAccStructBuilder.h>
#include <rtp/UniformGridAccStruct.h>

#include <rt/AabbIntersection.h>
#include <rt/Sphere.h>

using namespace rtp;

void printGridStats( const UniformGridAccStruct* grid )
{
	int32 nx, ny, nz;
	grid->getResolution( nx, ny, nz );

	// stats
	int32 minCellSize = INT_MAX;
	int32 minCellSizeCount = 0;
	int32 maxCellSize = -INT_MAX;
	int32 maxCellSizeCount = 0;
	float avgCellSize = 0.0f;

	for( int32 z = 0; z < nz; ++z )
	{
		for( int32 y = 0; y < ny; ++y )
		{
			for( int32 x = 0; x < nx; ++x )
			{
				const UniformGridAccStruct::Cell& cell = grid->at( x, y, z );

				// stats
				if( (int32)cell.size() <= minCellSize )
				{
					minCellSize = cell.size();
					++minCellSizeCount;
				}
				else if( (int32)cell.size() >= maxCellSize )
				{
					maxCellSize = cell.size();
					++maxCellSizeCount;
				}

				avgCellSize += (float)cell.size();
			}
		}
	}

	// divide by cell total
	avgCellSize /= (float)( nx * ny * nz );

	printf( "\n***** Uniform Grid *****\n" );
	printf( "numCells: %d, %d, %d (%d cells)\n", nx, ny, nz, nx * ny * nz );
	printf( "minCellSize: %d (%d cells)\n", minCellSize, minCellSizeCount );
	printf( "maxCellSize: %d (%d cells)\n", maxCellSize, maxCellSizeCount );
	printf( "averageCellSize: %5.4f\n", avgCellSize );
}

//////////////////////////////////////////////////////////////////////////

void UniformGridAccStructBuilder::buildGeometry( rt::Geometry* geometry )
{
	buildUniformGrid( geometry );

	// TODO: 28-2-2008
	// TODO: we tried this grid in order to build inside GPU like Particle Simulation from Waldemar
	// TODO: currently abandoned in favor of traditional grid, because it is at least 2x faster
	//buildCubeGrid( geometry );
}

//////////////////////////////////////////////////////////////////////////
// Private
//////////////////////////////////////////////////////////////////////////
void UniformGridAccStructBuilder::buildUniformGrid( rt::Geometry* geometry )
{
	// Compute scene box
	rt::Aabb bbox;
	bbox.buildFrom( &geometry->vertices[0], geometry->vertices.size() );

	// Add a tolerance to planar box dimension(s) to guarantee that grid has a volume > 0
	// Also avoid precision problems during triangle insertion, ray traversal and ray intersection
	bbox.scaleBy( 0.01f );

	// Create uniform grid
	UniformGridAccStruct* grid = new UniformGridAccStruct();
	geometry->accStruct = grid;

	// Compute total geometry volume
	vr::vec3f diagonal = bbox.maxv - bbox.minv;
	float V = ( diagonal.x * diagonal.y * diagonal.z );

	// TODO: improve this equation / heuristic
	// TODO: magic user-supplied number
	int32 k = 6;
	int32 triCount = geometry->triDesc.size();
	float factor = powf( (float)( k * triCount ) / V, 1.0f / 3.0f );

	// Compute actual number of cells in each dimension
	int32 nx = (int32)ceilf( diagonal.x * factor );
	int32 ny = (int32)ceilf( diagonal.y * factor );
	int32 nz = (int32)ceilf( diagonal.z * factor );

	// Store grid data
	grid->setBoundingBox( bbox );
	grid->setResolution( nx, ny, nz );

	// Reference cell box for triangle overlap test
	rt::Aabb cellBox;
	rt::Aabb triBox;

	// For each triangle, compute which cells it could overlap and check overlap only for them O(n)
	for( int32 t = 0; t < triCount; ++t )
	{
		// Set initial triangle Aabb to v0 and expand to include other 2 vertices
		const vr::vec3f& v0 = geometry->getVertex( t, 0 );
		const vr::vec3f& v1 = geometry->getVertex( t, 1 );
		const vr::vec3f& v2 = geometry->getVertex( t, 2 );
		triBox.minv = v0;
		triBox.maxv = v0;
		triBox.expandBy( v1 );
		triBox.expandBy( v2 );

		// Now that we have the triangle box, need to find which grid cells it overlaps
		const int32 xStart = grid->worldToVoxel( triBox.minv.x, RT_AXIS_X );
		const int32 yStart = grid->worldToVoxel( triBox.minv.y, RT_AXIS_Y );
		const int32 zStart = grid->worldToVoxel( triBox.minv.z, RT_AXIS_Z );

		const int32 xEnd = grid->worldToVoxel( triBox.maxv.x, RT_AXIS_X );
		const int32 yEnd = grid->worldToVoxel( triBox.maxv.y, RT_AXIS_Y );
		const int32 zEnd = grid->worldToVoxel( triBox.maxv.z, RT_AXIS_Z );

		// Loop over all cells overlapped by AABB and check if triangle actually overlaps any
		for( int32 z = zStart; z <= zEnd; ++z )
		{
			for( int32 y = yStart; y <= yEnd; ++y )
			{
				for( int32 x = xStart; x <= xEnd; ++x )
				{
					// Update cell box to check for triangle overlap
					cellBox.minv = bbox.minv + vr::vec3f( x, y, z ) * grid->getCellSize();
					cellBox.maxv = cellBox.minv + grid->getCellSize();

					//if( rt::AabbIntersection::triangleOverlaps( cellBox, v0, v1, v2 ) )
						grid->at( x, y, z ).push_back( t );
				}
			}
		}
	}

	// Print stats
	printGridStats( grid );
}

void UniformGridAccStructBuilder::buildCubeGrid( rt::Geometry* geometry )
{
	// Compute scene box
	rt::Aabb bbox;
	bbox.buildFrom( &geometry->vertices[0], geometry->vertices.size() );

	// Add a tolerance to planar box dimension(s) to guarantee that grid has a volume > 0
	// Also avoid precision problems during triangle insertion, ray traversal and ray intersection
	bbox.scaleBy( 0.01f );

	// Create uniform grid
	UniformGridAccStruct* grid = new UniformGridAccStruct();
	geometry->accStruct = grid;

	// Box dimensions
	vr::vec3f diagonal = bbox.maxv - bbox.minv;

	// Keep track of largest bounding sphere radius
	float maxRadius = -vr::Mathf::MAX_VALUE;
	float minRadius = vr::Mathf::MAX_VALUE;

	int32 triCount = geometry->triDesc.size();
	std::vector<rt::Sphere> spheres( triCount );

	// Assuming geometry triangles are of approximate same size
	for( int32 t = 0; t < triCount; ++t )
	{
		const vr::vec3f& v0 = geometry->getVertex( t, 0 );
		const vr::vec3f& v1 = geometry->getVertex( t, 1 );
		const vr::vec3f& v2 = geometry->getVertex( t, 2 );

		spheres[t].computeFrom( v0, v1, v2 );

		if( spheres[t].radius > maxRadius )
			maxRadius = spheres[t].radius;
		else if( spheres[t].radius < minRadius )
			minRadius = spheres[t].radius;
	}

	printf( "\n**** histogram ****\n" );
	printf( "minRadius: %5.4f\n", minRadius );
	printf( "maxRadius: %5.4f\n", maxRadius );

	// Compute histogram
	int32 numIntervals = 10;
	float intervalSize = ( maxRadius - minRadius ) / (float)numIntervals;
	std::vector<int32> histogram( numIntervals );

	for( uint32 i = 0; i < spheres.size(); ++i )
	{
		float radius = spheres[i].radius;

		for( int32 j = 1; j <= numIntervals; ++j )
		{
			if( radius <= ( minRadius + intervalSize * j ) )
			{
				++histogram[j-1];
				break;
			}
		}
	}

	// Use cell size based on histogram
	int32 largestCount = -vr::Mathf::MAX_VALUE;
	int32 bestIdx = histogram.size() - 1;

	// Print histogram
	for( uint32 i = 0; i < histogram.size(); ++i )
	{
		printf( "Between %5.4f and %5.4f: %6d\n", minRadius + intervalSize * i, minRadius + intervalSize * (i+1), histogram[i] );

		if( histogram[i] > largestCount )
		{
			largestCount = histogram[i];
			bestIdx = i;
		}
	}

	float bestRadius = minRadius + intervalSize * bestIdx;
	printf( "Sphere radius used: %5.4f\n", bestRadius );

	// Set cell size as the diameter of the largest triangle's bounding sphere
	float diameter = 2.0f * bestRadius;

	// Compute number of cells, rounding up
	int32 nx = (int32)ceilf( diagonal.x / diameter );
	int32 ny = (int32)ceilf( diagonal.y / diameter );
	int32 nz = (int32)ceilf( diagonal.z / diameter );

	// Update to rounded up diagonal
	diagonal.x = nx * diameter;
	diagonal.y = ny * diameter;
	diagonal.z = nz * diameter;

	// Extend the box to accommodate our required cell count and size
	bbox.maxv = bbox.minv + diagonal;

	grid->setBoundingBox( bbox );
	grid->setResolution( nx, ny, nz );

	// Set cell triangles according to their centers
	// Each triangle must only occupy a single cell!
	for( int32 t = 0; t < triCount; ++t )
	{
		const int32 x = grid->worldToVoxel( spheres[t].center.x, RT_AXIS_X );
		const int32 y = grid->worldToVoxel( spheres[t].center.y, RT_AXIS_Y );
		const int32 z = grid->worldToVoxel( spheres[t].center.z, RT_AXIS_Z );

		grid->at( x, y, z ).push_back( t );
	}

	// Print stats
	printGridStats( grid );

	// Code to check grid integrity

	// store which cell each triangle belong to
	std::vector<int32> triangleCellIds( triCount, -1 );

	for( int32 z = 0; z < nz; ++z )
	{
		for( int32 y = 0; y < ny; ++y )
		{
			for( int32 x = 0; x < nx; ++x )
			{
				const std::vector<int32>& cell = grid->at( x, y, z );

				int32 id = x + y*nx + z*nx*ny;

				for( uint32 t = 0; t < cell.size(); ++t )
				{
					int32& storeId = triangleCellIds[cell[t]];

					if( storeId != -1 )
					{
						printf( "duplicate triangle found!\n" );
						getc(stdin);
					}

					storeId = id;
				}
			}
		}
	}

	for( uint32 i = 0; i < triangleCellIds.size(); ++i )
	{
		if( triangleCellIds[i] == -1 )
		{
			printf( "triangle not stored in any cell!\n" );
			getc(stdin);
		}
	}
}
