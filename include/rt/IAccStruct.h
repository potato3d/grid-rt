#ifndef _RT_IACCSTRUCT_H_
#define _RT_IACCSTRUCT_H_

#include <rt/IPlugin.h>
#include <rt/Aabb.h>
#include <rt/Sample.h>

namespace rt {

class IAccStruct : public IPlugin
{
public:
	// Default implementation: do nothing
	virtual void clear();
	virtual void setBoundingBox( const rt::Aabb& bbox );
	virtual const rt::Aabb& getBoundingBox() const;

	// Ray must be transformed to local space prior to calling traceNearestGeometry
	virtual void traceNearestInstance( const std::vector<rt::Instance>& instances, rt::Sample& sample );

	// Ray is already transformed to instance local space
	virtual void traceNearestGeometry( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit );

	// Ray must be transformed to local space prior to calling traceNearestGeometry
	virtual bool traceAnyInstance( const std::vector<rt::Instance>& instances, rt::Sample& sample );

	// Ray is already transformed to instance local space
	virtual bool traceAnyGeometry( const rt::Instance& instance, rt::Ray& ray, rt::Hit& hit );

protected:
	rt::Aabb _bbox;
};

} // namespace rt

#endif // _RT_IACCSTRUCT_H_
