#include <rt/RayTriIntersection.h>
#include <rt/Geometry.h>

using namespace rt;

static const float HIT_EPSILON = 1e-4f;
uint32 RayTriIntersection::s_modulo[8] = { 0, 1, 2, 0, 1, 2, 0, 1 };

#define KU s_modulo[acc.k+1]
#define KV s_modulo[acc.k+2]

void RayTriIntersection::hitWald( const rt::TriAccel& acc, const rt::Ray& ray, rt::Hit& hit, float& bestDistance )
{
	// Start high-latency division as early as possible
	const float nd = 1.0f / ( ray.dir[acc.k] + acc.n_u * ray.dir[KU] + acc.n_v * ray.dir[KV] );
	const float f = nd * ( acc.n_d - ray.orig[acc.k] - acc.n_u * ray.orig[KU] - acc.n_v * ray.orig[KV] );

	// Check for valid distance
	// TODO: find a correct way to get rid of these epsilons and check scene06 for any errors
	if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
		return;

	// Compute hit point positions on uv plane
	const float hu = ray.orig[KU] + f * ray.dir[KU];
	const float hv = ray.orig[KV] + f * ray.dir[KV];

	// Check first barycentric coordinate
	const float lambda = hu * acc.b_nu + hv * acc.b_nv + acc.b_d;
	if( lambda < 0.0f )
		return;

	// Check second barycentric coordinate
	const float mue = hu * acc.c_nu + hv * acc.c_nv + acc.c_d;
	if( mue < 0.0f )
		return;

	// Check third barycentric coordinate
	const float psi = 1.0f - lambda - mue;
	if( psi < 0.0f )
		return;

	// Have a valid hit point here. Store it.
	bestDistance = f;
	hit.triangleId = acc.triangleId;
	hit.v0Coord = psi;
	hit.v1Coord = lambda;
	hit.v2Coord = mue;
}

void RayTriIntersection::hitMT1( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
							    float& bestDistance )
{
	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	/* find vectors for two edges sharing vert0 */
	const vr::vec3f edge1 = v1 - v0;
	const vr::vec3f edge2 = v2 - v0;

	/* begin calculating determinant - also used to calculate U parameter */
	const vr::vec3f pvec = ray.dir.cross( edge2 );

	/* if determinant is near zero, ray lies in plane of triangle */
	float det = edge1.dot( pvec );

	if( det > -HIT_EPSILON && det < HIT_EPSILON )
		return;

	const float inv_det = 1.0f / det;

	/* calculate distance from vert0 to ray origin */
	const vr::vec3f tvec = ray.orig - v0;

	/* calculate U parameter and test bounds */
	const float u = tvec.dot( pvec ) * inv_det;
	if( u < 0.0f || u > 1.0f )
		return;

	/* prepare to test V parameter */
	const vr::vec3f qvec = tvec.cross( edge1 );

	/* calculate V parameter and test bounds */
	const float v = ray.dir.dot( qvec ) * inv_det;
	if( v < 0.0f || u + v > 1.0f )
		return;

	/* calculate t, ray hits triangle */
	const float f = edge2.dot( qvec ) * inv_det;

	if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
		return;

	// Have a valid hit point here. Store it.
	bestDistance = f;
	hit.triangleId = triId;
	hit.v0Coord = 1.0f - (u + v);
	hit.v1Coord = u;
	hit.v2Coord = v;
}

void RayTriIntersection::hitMT2( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
							    float& bestDistance )
{
	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	vr::vec3f qvec;
	float u;
	float v;

	/* find vectors for two edges sharing vert0 */
	const vr::vec3f edge1 = v1 - v0;
	const vr::vec3f edge2 = v2 - v0;

	/* begin calculating determinant - also used to calculate U parameter */
	const vr::vec3f pvec = ray.dir.cross( edge2 );

	/* if determinant is near zero, ray lies in plane of triangle */
	float det = edge1.dot( pvec );

	if( det > HIT_EPSILON )
	{
		/* calculate distance from vert0 to ray origin */
		const vr::vec3f tvec = ray.orig - v0;

		/* calculate U parameter and test bounds */
		u = tvec.dot( pvec );
		if( u < 0.0 || u > det )
			return;

		/* prepare to test V parameter */
		qvec = tvec.cross( edge1 );

		/* calculate V parameter and test bounds */
		v = ray.dir.dot( qvec );
		if( v < 0.0 || u + v > det )
			return;
	}
	else if( det < -HIT_EPSILON )
	{
		/* calculate distance from vert0 to ray origin */
		const vr::vec3f tvec = ray.orig - v0;

		/* calculate U parameter and test bounds */
		u = tvec.dot( pvec );
		if( u > 0.0 || u < det )
			return;

		/* prepare to test V parameter */
		qvec = tvec.cross( edge1 );

		/* calculate V parameter and test bounds */
		v = ray.dir.dot( qvec );
		if( v > 0.0 || u + v < det )
			return;
	}
	else
	{
		/* ray is parallel to the plane of the triangle */
		return;
	}

	const float inv_det = 1.0 / det;

	/* calculate t, ray hits triangle */
	const float f = edge2.dot( qvec ) * inv_det;

	if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
		return;

	// Have a valid hit point here. Store it.
	bestDistance = f;
	hit.triangleId = triId;
	hit.v0Coord = 1.0f - (u + v);
	hit.v1Coord = u;
	hit.v2Coord = v;
}

void RayTriIntersection::hitMT3( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
							    float& bestDistance )
{
	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	vr::vec3f qvec;
	float u;
	float v;

	/* find vectors for two edges sharing vert0 */
	const vr::vec3f edge1 = v1 - v0;
	const vr::vec3f edge2 = v2 - v0;

	/* begin calculating determinant - also used to calculate U parameter */
	const vr::vec3f pvec = ray.dir.cross( edge2 );

	/* if determinant is near zero, ray lies in plane of triangle */
	float det = edge1.dot( pvec );

	/* calculate distance from vert0 to ray origin */
	const vr::vec3f tvec = ray.orig - v0;

	const float inv_det = 1.0 / det;

	if( det > HIT_EPSILON )
	{
		/* calculate U parameter and test bounds */
		u = tvec.dot( pvec );
		if( u < 0.0 || u > det )
			return;

		/* prepare to test V parameter */
		qvec = tvec.cross( edge1 );

		/* calculate V parameter and test bounds */
		v = ray.dir.dot( qvec );
		if( v < 0.0 || u + v > det )
			return;
	}
	else if( det < -HIT_EPSILON )
	{
		/* calculate U parameter and test bounds */
		u = tvec.dot( pvec );
		if( u > 0.0 || u < det )
			return;

		/* prepare to test V parameter */
		qvec = tvec.cross( edge1 );

		/* calculate V parameter and test bounds */
		v = ray.dir.dot( qvec );
		if( v > 0.0 || u + v < det )
			return;
	}
	else
	{
		/* ray is parallel to the plane of the triangle */
		return;
	}

	/* calculate t, ray hits triangle */
	const float f = edge2.dot( qvec ) * inv_det;

	if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
		return;

	// Have a valid hit point here. Store it.
	bestDistance = f;
	hit.triangleId = triId;
	hit.v0Coord = 1.0f - (u + v);
	hit.v1Coord = u;
	hit.v2Coord = v;
}

void RayTriIntersection::hitMT4( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
							    float& bestDistance )
{
	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	vr::vec3f qvec;
	float u;
	float v;

	/* find vectors for two edges sharing vert0 */
	const vr::vec3f edge1 = v1 - v0;
	const vr::vec3f edge2 = v2 - v0;

	/* begin calculating determinant - also used to calculate U parameter */
	const vr::vec3f pvec = ray.dir.cross( edge2 );

	/* if determinant is near zero, ray lies in plane of triangle */
	float det = edge1.dot( pvec );

	/* calculate distance from vert0 to ray origin */
	const vr::vec3f tvec = ray.orig - v0;

	/* prepare to test V parameter */
	qvec = tvec.cross( edge1 );

	const float inv_det = 1.0 / det;

	if( det > HIT_EPSILON )
	{
		/* calculate U parameter and test bounds */
		u = tvec.dot( pvec );
		if( u < 0.0 || u > det )
			return;

		/* calculate V parameter and test bounds */
		v = ray.dir.dot( qvec );
		if( v < 0.0 || u + v > det )
			return;
	}
	else if( det < -HIT_EPSILON )
	{
		/* calculate U parameter and test bounds */
		u = tvec.dot( pvec );
		if( u > 0.0 || u < det )
			return;

		/* calculate V parameter and test bounds */
		v = ray.dir.dot( qvec );
		if( v > 0.0 || u + v < det )
			return;
	}
	else
	{
		/* ray is parallel to the plane of the triangle */
		return;
	}

	/* calculate t, ray hits triangle */
	const float f = edge2.dot( qvec ) * inv_det;

	if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
		return;

	// Have a valid hit point here. Store it.
	bestDistance = f;
	hit.triangleId = triId;
	hit.v0Coord = 1.0f - (u + v);
	hit.v1Coord = u;
	hit.v2Coord = v;
}

void RayTriIntersection::hitMT5( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
							    float& bestDistance )
{
	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	vr::vec3f qvec;
	float u;
	float v;

	/* find vectors for two edges sharing vert0 */
	const vr::vec3f edge1 = v1 - v0;
	const vr::vec3f edge2 = v2 - v0;

	/* begin calculating determinant - also used to calculate U parameter */
	const vr::vec3f pvec = ray.dir.cross( edge2 );

	/* if determinant is near zero, ray lies in plane of triangle */
	float det = edge1.dot( pvec );

	/* calculate distance from vert0 to ray origin */
	const vr::vec3f tvec = ray.orig - v0;

	/* prepare to test V parameter */
	qvec = tvec.cross( edge1 );

	if( det > HIT_EPSILON )
	{
		/* calculate U parameter and test bounds */
		u = tvec.dot( pvec );
		if( u < 0.0 || u > det )
			return;

		/* calculate V parameter and test bounds */
		v = ray.dir.dot( qvec );
		if( v < 0.0 || u + v > det )
			return;
	}
	else if( det < -HIT_EPSILON )
	{
		/* calculate U parameter and test bounds */
		u = tvec.dot( pvec );
		if( u > 0.0 || u < det )
			return;

		/* calculate V parameter and test bounds */
		v = ray.dir.dot( qvec );
		if( v > 0.0 || u + v < det )
			return;
	}
	else
	{
		/* ray is parallel to the plane of the triangle */
		return;
	}

	const float inv_det = 1.0 / det;

	/* calculate t, ray hits triangle */
	const float f = edge2.dot( qvec ) * inv_det;

	if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
		return;

	// Have a valid hit point here. Store it.
	bestDistance = f;
	hit.triangleId = triId;
	hit.v0Coord = 1.0f - (u + v);
	hit.v1Coord = u;
	hit.v2Coord = v;
}

void RayTriIntersection::hitMT6( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
							    float& bestDistance )
{
	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	vr::vec3f qvec;
	float u;
	float v;

	/* find vectors for two edges sharing vert0 */
	const vr::vec3f edge1 = v1 - v0;
	const vr::vec3f edge2 = v2 - v0;

	/* begin calculating determinant - also used to calculate U parameter */
	const vr::vec3f pvec = ray.dir.cross( edge2 );

	/* if determinant is near zero, ray lies in plane of triangle */
	float det = edge1.dot( pvec );

	/* calculate distance from vert0 to ray origin */
	const vr::vec3f tvec = ray.orig - v0;

	/* prepare to test V parameter */
	qvec = tvec.cross( edge1 );

	/* calculate U parameter and test bounds */
	u = tvec.dot( pvec );

	/* calculate V parameter and test bounds */
	v = ray.dir.dot( qvec );

	int32 detFlag = (det >= 0);

	if( !( detFlag ^ ( u < 0.0f ) && detFlag ^ ( u > det ) ) )
		return;

	const float inv_det = 1.0 / det;

	/* calculate t, ray hits triangle */
	const float f = edge2.dot( qvec ) * inv_det;

	if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
		return;

	// Have a valid hit point here. Store it.
	bestDistance = f;
	hit.triangleId = triId;
	hit.v0Coord = 1.0f - (u + v);
	hit.v1Coord = u;
	hit.v2Coord = v;
}

void RayTriIntersection::hitMT7( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
							    float& bestDistance )
{
	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	/* find vectors for two edges sharing vert0 */
	const vr::vec3f edge1 = v1 - v0;
	const vr::vec3f edge2 = v2 - v0;

	/* begin calculating determinant - also used to calculate U parameter */
	const vr::vec3f pvec = ray.dir.cross( edge2 );

	/* if determinant is near zero, ray lies in plane of triangle */
	float det = edge1.dot( pvec );

	if( det > -HIT_EPSILON && det < HIT_EPSILON )
		return;

	const float inv_det = 1.0f / det;

	/* calculate distance from vert0 to ray origin */
	const vr::vec3f tvec = ray.orig - v0;

	/* prepare to test V parameter */
	const vr::vec3f qvec = tvec.cross( edge1 );

	/* calculate t, ray hits triangle */
	const float f = edge2.dot( qvec ) * inv_det;

	if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
		return;

	/* calculate U parameter and test bounds */
	const float u = tvec.dot( pvec ) * inv_det;
	if( u < 0.0 || u > 1.0 )
		return;

	/* calculate V parameter and test bounds */
	const float v = ray.dir.dot( qvec ) * inv_det;
	if( v < 0.0 || u + v > 1.0 )
		return;

	// Have a valid hit point here. Store it.
	bestDistance = f;
	hit.triangleId = triId;
	hit.v0Coord = 1.0f - (u + v);
	hit.v1Coord = u;
	hit.v2Coord = v;
}

void RayTriIntersection::hitTest( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
								 float& bestDistance )
{
	vr::vec3f u, v, n;             // triangle vectors
	vr::vec3f w0, w;          // ray vectors
	float     r, a, b;             // params to calc ray-plane hit

	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	// get triangle edge vectors and plane normal
	u = v1 - v0;
	v = v2 - v0;
	n = u.cross( v );             // cross product

	w0 = ray.orig - v0;
	a = -n.dot( w0 );
	b = n.dot( ray.dir );
	if( fabs(b) < HIT_EPSILON ) // ray is parallel to triangle plane
		return;

	// get hit point of ray with triangle plane
	r = a / b;
	if( ( r >= bestDistance ) || ( r < ray.tnear - HIT_EPSILON ) || ( r > ray.tfar + HIT_EPSILON ) )
		return;

	vr::vec3f I = ray.orig + ray.dir * r;           // hit point of ray and plane

	// is I inside T?
	float    uu, uv, vv, wu, wv, D;
	uu = u.dot(u);
	uv = u.dot(v);
	vv = v.dot(v);
	w = I - v0;
	wu = w.dot(u);
	wv = w.dot(v);
	D = uv * uv - uu * vv;

	// get and test parametric coords
	float s, t;
	s = (uv * wv - vv * wu) / D;
	if (s < 0.0f || s > 1.0f)        // I is outside T
		return;
	t = (uv * wu - uu * wv) / D;
	if (t < 0.0f || (s + t) > 1.0f)  // I is outside T
		return;

	// Have a valid hit point here. Store it.
	bestDistance = r;
	hit.triangleId = triId;
	hit.v0Coord = 1.0f - (s + t);
	hit.v1Coord = s;
	hit.v2Coord = t;
}

void RayTriIntersection::hitChirkov( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
								    float& bestDistance )
{
	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	vr::vec3f end = ray.orig + ray.dir * ray.tfar;

	float e0x = v1.x - v0.x;
	float e0y = v1.y - v0.y;
	float e0z = v1.z - v0.z;
	float e1x = v2.x - v0.x;
	float e1y = v2.y - v0.y;
	float e1z = v2.z - v0.z;
	float normx = e0y * e1z - e0z * e1y;
	float normy = e0z * e1x - e0x * e1z;
	float normz = e0x * e1y - e0y * e1x;
	float pd = normx*v0.x + normy*v0.y + normz*v0.z;

	float signSrc = normx*ray.orig.x + normy*ray.orig.y + normz*ray.orig.z - pd;
	float signDst = normx*end.x + normy*end.y + normz*end.z - pd;
	if(signSrc*signDst > 0.0f)
		return;

	float SQ = 0.57735f;//sqrtf(1.0f/3.0f);
	float len = (normx*normx + normy*normy + normz*normz)*SQ;

	float d = signSrc - signDst;
	if(fabs(normx)>len)
	{
		float diry = end.y - ray.orig.y;
		float dirz = end.z - ray.orig.z;
		float basey = ray.orig.y - v0.y;
		float basez = ray.orig.z - v0.z;

		float adelx = signSrc*(e0y * dirz - e0z * diry);
		if( (adelx + d*(e0y*basez - e0z*basey)) * ( signSrc*(diry*e1z - dirz*e1y) + d*(basey*e1z - basez*e1y)) > 0.0)
		{
			float e2y = v1.y - v2.y;
			float e2z = v1.z - v2.z;
			basey = ray.orig.y - v1.y;
			basez = ray.orig.z - v1.z;
			if( (adelx + d*(e0y*basez - e0z*basey)) * ( signSrc*(diry*e2z - dirz*e2y) + d*(basey*e2z - basez*e2y)) > 0.0)	
			{
				// Have a valid hit point here. Store it.
				float f = - signSrc / (ray.dir.x * normx + ray.dir.y * normy + ray.dir.z * normz);

				if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
					return;

				bestDistance = f;
				hit.triangleId = triId;
				hit.v0Coord = 1.0f;
				hit.v1Coord = 1.0f;
				hit.v2Coord = 1.0f;
				return;
			}
		}
	}
	else
	{
		if(fabs(normy)>len)
		{
			float dirx = end.x - ray.orig.x;
			float dirz = end.z - ray.orig.z;
			float basex = ray.orig.x - v0.x;
			float basez = ray.orig.z - v0.z;
			float adely = signSrc*(e0z * dirx - e0x * dirz);
			if( (adely + d*(e0z*basex - e0x*basez)) * ( signSrc*(dirz*e1x - dirx*e1z) + d*(basez*e1x - basex*e1z)) > 0.0)
			{
				float e2x = v1.x - v2.x;
				float e2z = v1.z - v2.z;
				basex = ray.orig.x - v1.x;
				basez = ray.orig.z - v1.z;
				if( (adely + d*(e0z*basex - e0x*basez)) * ( signSrc*(dirz*e2x - dirx*e2z) + d*(basez*e2x - basex*e2z)) > 0.0)	
				{
					// Have a valid hit point here. Store it.
					float f = - signSrc / (ray.dir.x * normx + ray.dir.y * normy + ray.dir.z * normz);

					if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
						return;

					bestDistance = f;
					hit.triangleId = triId;
					hit.v0Coord = 1.0f;
					hit.v1Coord = 1.0f;
					hit.v2Coord = 1.0f;
					return;
				}
			}
		}
		else
		{
			float dirx = end.x - ray.orig.x;
			float diry = end.y - ray.orig.y;
			float basex = ray.orig.x - v0.x;
			float basey = ray.orig.y - v0.y;
			float adelz = signSrc*(e0x * diry - e0y * dirx);

			if( (adelz + d*(e0x*basey - e0y*basex)) * ( signSrc*(dirx*e1y - diry*e1x) + d*(basex*e1y - basey*e1x)) > 0.0)
			{
				// Have a valid hit point here. Store it.
				float f = - signSrc / (ray.dir.x * normx + ray.dir.y * normy + ray.dir.z * normz);

				if( ( f >= bestDistance ) || ( f < ray.tnear - HIT_EPSILON ) || ( f > ray.tfar + HIT_EPSILON ) )
					return;

				bestDistance = f;
				hit.triangleId = triId;
				hit.v0Coord = 1.0f;
				hit.v1Coord = 1.0f;
				hit.v2Coord = 1.0f;
				return;
			}
		}
	}
}

void RayTriIntersection::hitHalfSpace( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
										 float& bestDistance )
{
	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	const vr::vec3f normal = ( v1 - v0 ).cross( v2 - v0 );

	// 1. Find hition with triangle plane
	const float t = - ( ray.orig - v0 ).dot( normal ) / ray.dir.dot( normal );

	if( ( t >= bestDistance ) || ( t < ray.tnear - HIT_EPSILON ) || ( t > ray.tfar + HIT_EPSILON ) )
		return;

	// 2. Compute hition point
	const vr::vec3f x = ray.orig + ray.dir * t;

	// 3. Check if x is inside 3 half-spaces
	const float a = ( v1 - v0 ).cross( x - v0 ).dot( normal );
	const float b = ( v2 - v1 ).cross( x - v1 ).dot( normal );
	const float c = ( v0 - v2 ).cross( x - v2 ).dot( normal );

	if( a >= 0 && b >= 0 && c >= 0 )
	{
		bestDistance = t;
		hit.triangleId = triId;
		hit.v0Coord = 1.0f;
		hit.v1Coord = 1.0f;
		hit.v2Coord = 1.0f;
	}
}

void RayTriIntersection::hitSignedVolume( const rt::Geometry& geom, uint32 triId, const rt::Ray& ray, rt::Hit& hit, 
										 float& bestDistance )
{
	const vr::vec3f& v0 = geom.getVertex( triId, 0 );
	const vr::vec3f& v1 = geom.getVertex( triId, 1 );
	const vr::vec3f& v2 = geom.getVertex( triId, 2 );

	const vr::vec3f e1 = v1 - v0;
	const vr::vec3f e2 = v2 - v0;

	const float det = ray.dir.dot( e1.cross( e2 ) );
	const vr::vec3f tvec = ray.orig - v0;
	const float u = ray.dir.dot( e1.cross( tvec ) );

	if ( u < 0 || u > det )
		return;

	const float v = ray.dir.dot( tvec.cross( e2 ) );

	if ( v < 0 || u + v > det )
		return;

	// TODO: need closest distance! :(
	//bestDistance = t;
	hit.triangleId = triId;
	hit.v0Coord = 1.0f;
	hit.v1Coord = 1.0f;
	hit.v2Coord = 1.0f;
}

#undef KU
#undef KV
