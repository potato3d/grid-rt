#ifndef _RT_AABB_H_
#define _RT_AABB_H_

#include <rt/common.h>
#include <vr/vec3.h>

namespace rt {

struct Aabb
{
	Aabb();

	void buildFrom( const vr::vec3f* vertices, int32 vertexCount );

	void expandBy( const vr::vec3f* vertices, int32 vertexCount );
	void expandBy( const vr::vec3f& vertex );
	void expandBy( const Aabb& other );

	// Apply scale% in all directions, negative values will shrink the box
	// If box has a planar dimension, it will be expanded by given tolerance
	// i.e. scale = 0.1 expands the Aabb by 10% in all directions, effectively increasing each dimension by 20% total
	void scaleBy( float scale, float tolerance = 0.01f );

	bool isPlanar( RTenum axis ) const;
	bool isDegenerate() const;
	float computeSurfaceArea() const;

	/*
	*  Vertices are computed as follows:
	*     7+------+6
	*     /|     /|      y
	*    / |    / |      |
	*   / 3+---/--+2     |
	* 4+------+5 /       *---x
	*  | /    | /       /
	*  |/     |/       z
	* 0+------+1      
	*
	* Assumes array is pre-allocated!
	*/
	void computeVertices( vr::vec3f* vertices ) const;

	vr::vec3f minv;
	vr::vec3f maxv;
};

} // namespace rt

#endif // _RT_AABB_H_
