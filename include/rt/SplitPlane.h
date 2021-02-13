#ifndef _RT_SPLITPLANE_H_
#define _RT_SPLITPLANE_H_

#include <rt/common.h>

namespace rt {

struct SplitPlane
{
	inline bool operator==( const SplitPlane& other ) const;

	RTenum axis;
	float position;
};

inline bool SplitPlane::operator==( const SplitPlane& other ) const
{
	return ( axis == other.axis ) && ( position == other.position );
}


} // namespace rt

#endif // _RT_SPLITPLANE_H_
