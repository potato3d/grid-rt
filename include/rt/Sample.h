#ifndef _RT_SAMPLE_H_
#define _RT_SAMPLE_H_

#include <rt/common.h>
#include <rt/Ray.h>
#include <rt/Hit.h>

namespace rt {

class Sample
{
public:
	static void reflectDirectionVector( const vr::vec3f& incident, const vr::vec3f& normal, vr::vec3f& reflected );

	// Use camera to setup primary ray sample.
	// Initializes ray origin and direction.
	// Resets ray recursion depth.
	void initPrimaryRay( float x, float y );

	// Initialize sample information for querying light radiance samples.
	// Hit-position and shading normal must have been previously computed in base.
	// TODO: shading normal is used to displace the position and avoid precision problems for shadow rays
	void initLightRay( const Sample& base );

	// Initialize sample information for shadow rays.
	// Given sample must have been previously initialized as a light sample.
	// If surface points away from light, returns false and sample is invalid.
	// Else returns true and sample is valid.
	// Hit-position and shading normal must have been previously computed in light sample.
	bool initShadowRay( const vr::vec3f& directionTowardsLight, float rayMaxDistance );

	// Initialize sample information for reflection rays.
	// Hit-position and shading normal must have been previously computed in base.
	void initReflectionRay( const Sample& base );

	// Initialize sample information for refraction rays.
	// Uses shading normal to determine if ray is entering or exiting the object.
	// Computes the critical angle between the medium index and the given refraction index.
	// If there is total internal reflection, returns false and refraction sample is invalid.
	// Else returns true and refraction sample is valid.
	// Hit-position and shading normal must have been previously computed in sample.
	bool initRefractionRay( const Sample& base, float refractionIndex );

	// Compute hit position from current ray and hit distance
	void computeHitPosition();

	// Compute interpolated shading normal
	// Store it in normal
	void computeShadingNormal();

	// Compute interpolated shading color
	// Store it in color
	void computeShadingColor();

	// Compute interpolated texture coordinates
	void computeTexCoords( vr::vec3f& texCoords );

	// Get flat triangle normal from cross product of two triangle edges
	// Store it in normal
	void computeFlatNormal();

	// Uses dot product between shading normal and ray direction to determine triangle facing
	// Shading normal must have been previously computed in sample.
	bool isFrontFace() const;

	// Compute normalized vector for shading specular reflections.
	// Uses ray origin as viewpoint.
	// Hit-position and shading normal must have been previously computed in sample.
	void computeSpecularVector( vr::vec3f& v );

	// Returns whether maximum ray recursion depth has been reached or not
	bool stopRayRecursion() const;

	Ray ray;
	Hit hit;
	vr::vec3f color;
	uint32 recursionDepth;

	// Computable attributes necessary for inter-shader communication.
	// Prerequisite for several methods of this class.
	vr::vec3f hitPosition;
	vr::vec3f normal;
};

} // namespace rt

#endif // _RT_SAMPLE_H_
