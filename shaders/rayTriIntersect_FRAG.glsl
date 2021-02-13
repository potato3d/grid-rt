#version 110

uniform vec3 u_rayOrigin;

varying vec3 v_rayDir;
varying vec3 v_v0;
varying vec3 v_v1;
varying vec3 v_v2;

const float INTERSECT_EPSILON = 0.0001;

void main( void ) 
{
	/* find vectors for two edges sharing vert0 */
	vec3 edge1 = v_v1 - v_v0;
	vec3 edge2 = v_v2 - v_v0;

	/* begin calculating determinant - also used to calculate U parameter */
	vec3 pvec = cross( v_rayDir, edge2 );

	/* if determinant is near zero, ray lies in plane of triangle */
	float det = dot( edge1, pvec );

	if( det > -INTERSECT_EPSILON && det < INTERSECT_EPSILON )
		discard;

	float inv_det = 1.0 / det;

	/* calculate distance from vert0 to ray origin */
	vec3 tvec = u_rayOrigin - v_v0;

	/* calculate U parameter and test bounds */
	float u = dot( tvec, pvec ) * inv_det;
	if( u < 0.0 || u > 1.0 )
		discard;

	/* prepare to test V parameter */
	vec3 qvec = cross( tvec, edge1 );

	/* calculate V parameter and test bounds */
	float v = dot( v_rayDir, qvec ) * inv_det;
	if( v < 0.0 || u + v > 1.0 )
		discard;

	/* calculate t, ray intersects triangle */
	float f = dot( edge2, qvec ) * inv_det;

	if( f < 0.0 )
		discard;

	// Have a valid hit point here. Color it in blue.
	gl_FragColor = vec4( 0.0, 0.0, 1.0, 1.0 );
	//gl_FragDepth = f;
}

// This was the inner triangle intersection used in the main ray tracing shader
void hitMT( /* required params */ )
{
	// get vertices

	/* find vectors for two edges sharing vert0 */
	vec3 edge1 = v1 - v0;
	vec3 edge2 = v2 - v0;

	/* begin calculating determinant - also used to calculate U parameter */
	vec3 pvec = cross( v_rayDir, edge2 );

	/* if determinant is near zero, ray lies in plane of triangle */
	float det = dot( edge1, pvec );

	if( det > -HIT_EPSILON && det < HIT_EPSILON )
		return;

	float inv_det = 1.0 / det;

	/* calculate distance from vert0 to ray origin */
	vec3 tvec = u_rayOrig - v0;

	/* calculate U parameter and test bounds */
	float u = dot( tvec, pvec ) * inv_det;
	if( u < 0.0 || u > 1.0 )
		return;

	/* prepare to test V parameter */
	vec3 qvec = cross( tvec, edge1 );

	/* calculate V parameter and test bounds */
	float v = dot( v_rayDir, qvec ) * inv_det;
	if( v < 0.0 || u + v > 1.0 )
		return;

	/* calculate t, ray intersects triangle */
	float f = dot( edge2, qvec ) * inv_det;

	if( ( f >= bestDistance ) || ( f < tnear - HIT_EPSILON ) || ( f > tfar + HIT_EPSILON ) )
		return;

	// Have a valid hit point here.
	bestDistance = f;
	hit.triangleId = triangleId;
	hit.v1Coord = u;
	hit.v2Coord = v;
}

// This was the version used in the main ray tracing shader
bool rayHitsBoundingBox( inout float rayNear, inout float rayFar )
{
    // TODO: optimize, use language functions, etc
    float tNear;
	float tFar;

    /************ X AXIS *************/
	// Update interval for ith bounding box slab
	tNear = ( u_boxMin.x - u_rayOrig.x ) * g_rayInvDir.x;
	tFar  = ( u_boxMax.x - u_rayOrig.x ) * g_rayInvDir.x;

	// Update parametric interval from slab intersection tnear/tfar
	rayNear = max( rayNear, min( tNear, tFar ) );
	rayFar = min( rayFar, max( tNear, tFar ) );
	
	/************ Y AXIS *************/
	// Update interval for ith bounding box slab
	tNear = ( u_boxMin.y - u_rayOrig.y ) * g_rayInvDir.y;
	tFar  = ( u_boxMax.y - u_rayOrig.y ) * g_rayInvDir.y;

	// Update parametric interval from slab intersection tnear/tfar
	rayNear = max( rayNear, min( tNear, tFar ) );
	rayFar = min( rayFar, max( tNear, tFar ) );
	
	/************ Z AXIS *************/
	// Update interval for ith bounding box slab
	tNear = ( u_boxMin.z - u_rayOrig.z ) * g_rayInvDir.z;
	tFar  = ( u_boxMax.z - u_rayOrig.z ) * g_rayInvDir.z;

	// Update parametric interval from slab intersection tnear/tfar
	rayNear = max( rayNear, min( tNear, tFar ) );
	rayFar = min( rayFar, max( tNear, tFar ) );

	return ( rayNear <= rayFar );
}

// Original traversal code
void originalTraversal()
{
	if( tMax.x < tMax.y && tMax.x < tMax.z )
	{
		cellCoords.x += rayStep.x;
		tMax.x += tDelta.x;
	}
	else if( tMax.y < tMax.z )
	{
		cellCoords.y += rayStep.y;
		tMax.y += tDelta.y;
	}
	else
	{
		cellCoords.z += rayStep.z;
		tMax.z += tDelta.z;
	}
}
