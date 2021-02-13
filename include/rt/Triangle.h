#ifndef _RT_TRIANGLE_H_
#define _RT_TRIANGLE_H_

#include <rt/common.h>
#include <vr/vec3.h>

// Forward declarations
namespace rt {

	class IMaterial;
}

namespace rt {

struct TriDesc
{
	TriDesc();

	uint32 v[3];
	rt::IMaterial* material;
};

struct TriAccel
{
	void buildFrom( const vr::vec3f& v0, const vr::vec3f& v1, const vr::vec3f& v2 );
	bool valid() const;

	// first 16 byte half cache line
	RTenum k; // projection dimension
	// plane:
	float n_u; // normal.u / normal.k
	float n_v; // normal.v / normal.k
	float n_d; // constant of plane equation

	// second 16 byte half cache line
	// line equation for line ac
	float b_nu;
	float b_nv;
	float b_d;
	uint32 pad; // pad to next cache line

	// third 16 byte half cache line
	// line equation for line ab
	float c_nu;
	float c_nv;
	float c_d;
	uint32 triangleId; // pad to 48 bytes for cache alignment purposes
};

} // namespace rt

#endif // _RT_TRIANGLE_H_
