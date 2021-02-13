#ifndef _RTC_CUDAMATH_H_
#define _RTC_CUDAMATH_H_

#include <float.h>
#include <math_functions.h>
#include <vector_functions.h>

// Auxiliary constructors
static inline __host__ __device__ float3 make_float3( float value )
{
	return make_float3( value, value, value );
}

// Math functions and operators to be used with vector types.
static inline __host__ __device__ float3 operator *(float3 a, float3 b)
{
    return make_float3(a.x*b.x, a.y*b.y, a.z*b.z);
}

static inline __host__ __device__ float3 operator *(float f, float3 v)
{
    return make_float3(v.x*f, v.y*f, v.z*f);
}

static inline __host__ __device__ float3 operator *(float3 v, float f)
{
    return make_float3(v.x*f, v.y*f, v.z*f);
}

static inline __host__ __device__ float3 operator +(float3 a, float3 b)
{
    return make_float3(a.x+b.x, a.y+b.y, a.z+b.z);
}

static inline __host__ __device__ void operator +=(float3 & b, float3 a)
{
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
}

static inline __host__ __device__ float3 operator -(float3 v)
{
	return make_float3( -v.x, -v.y, -v.z );
}

static inline __host__ __device__ float3 operator -(float3 a, float3 b)
{
    return make_float3(a.x-b.x, a.y-b.y, a.z-b.z);
}

static inline __host__ __device__ void operator -=(float3 & b, float3 a)
{
    b.x -= a.x;
    b.y -= a.y;
    b.z -= a.z;
}

static inline __host__ __device__ float3 operator /(float3 a, float3 b)
{
	return make_float3(a.x/b.x, a.y/b.y, a.z/b.z);
}

static inline __host__ __device__ float3 operator /(float3 v, float f)
{
    float inv = 1.0f / f;
    return v * inv;
}

static inline __host__ __device__ void operator /=(float3 & b, float f)
{
    float inv = 1.0f / f;
    b.x *= inv;
    b.y *= inv;
    b.z *= inv;
}

static inline __host__ __device__ float dot(float3 a, float3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline __host__ __device__ float dot(float4 a, float4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline __host__ __device__ float3 cross( float3 a, float3 b )
{
	return make_float3(	a.y*b.z - a.z*b.y,
					    a.z*b.x - a.x*b.z,
						a.x*b.y - a.y*b.x );
}

static inline __host__ __device__ float clamp(float f, float a, float b)
{
    return max(a, min(f, b));
}

static inline __host__ __device__ float3 clamp(float3 v, float a, float b)
{
    return make_float3(clamp(v.x, a, b), clamp(v.y, a, b), clamp(v.z, a, b));
}

static inline __host__ __device__ float3 clamp(float3 v, float3 a, float3 b)
{
    return make_float3(clamp(v.x, a.x, b.x), clamp(v.y, a.y, b.y), clamp(v.z, a.z, b.z));
}

static inline __host__ __device__ float3 normalize(float3 v)
{
    float invLen = 1.0f / sqrtf(dot(v, v));
    return make_float3(v.x * invLen, v.y * invLen, v.z * invLen);
}

static inline __host__ __device__ int3 operator -(int3 a, int3 b)
{
	return make_int3(a.x-b.x, a.y-b.y, a.z-b.z);
}

static inline __host__ __device__ int clamp(int f, int a, int b)
{
	return max(a, min(f, b));
}

static inline __host__ __device__ int3 clamp(int3 v, int3 a, int3 b)
{
	return make_int3(clamp(v.x, a.x, b.x), clamp(v.y, a.y, b.y), clamp(v.z, a.z, b.z));
}

static inline __host__ __device__ float3 abs( float3 v )
{
	return make_float3( abs(v.x), abs(v.y), abs(v.z) );
}

static inline __host__ __device__ float3 min( float3 a, float3 b )
{
	return make_float3( min( a.x, b.x ), min( a.y, b.y ), min( a.z, b.z ) );
}

static inline __host__ __device__ float3 max( float3 a, float3 b )
{
	return make_float3( max( a.x, b.x ), max( a.y, b.y ), max( a.z, b.z ) );
}

static inline __host__ __device__ float3 not( float3 v )
{
	return make_float3( !v.x, !v.y, !v.z );
}

static inline __host__ __device__ float3 floor( float3 v )
{
	return make_float3( floor(v.x), floor(v.y), floor(v.z) );
}

static inline __host__ __device__ float3 step( float3 edge, float3 v )
{
	return make_float3( v.x >= edge.x, v.y >= edge.y, v.z >= edge.z );
}

static inline __host__ __device__ float3 rcp( float3 v )
{
	return make_float3( 1.0f / v.x, 1.0f / v.y, 1.0f / v.z );
}

// Use power method to find the first eigenvector.
// http://www.miislita.com/information-retrieval-tutorial/matrix-tutorial-3-eigenvalues-eigenvectors.html
static inline __host__ __device__ float3 firstEigenVector( float matrix[6] )
{
    // 8 iterations seems to be more than enough.

    float3 v = make_float3(1.0f, 1.0f, 1.0f);
    for(int i = 0; i < 8; i++) {
        float x = v.x * matrix[0] + v.y * matrix[1] + v.z * matrix[2];
        float y = v.x * matrix[1] + v.y * matrix[3] + v.z * matrix[4];
        float z = v.x * matrix[2] + v.y * matrix[4] + v.z * matrix[5];
        float m = max(max(x, y), z);        
        float iv = 1.0f / m;
        #if __DEVICE_EMULATION__
        if (m == 0.0f) iv = 0.0f;
        #endif
        v = make_float3(x*iv, y*iv, z*iv);
    }

    return v;
}

static inline __device__ float3 bestFitLine(const float3 * colors)
{
#if __DEVICE_EMULATION__

    // Compute covariance matrix of the given colors.
    float3 center = make_float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 16; i++)
    {
        center += colors[i];
    }
    center /= 16.0f;

    float covariance[6] = {0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 16; i++)
    {
        float3 a = colors[i] - center;
        covariance[0] += a.x * a.x;
        covariance[1] += a.x * a.y;
        covariance[2] += a.x * a.z;
        covariance[3] += a.y * a.y;
        covariance[4] += a.y * a.z;
        covariance[5] += a.z * a.z;
    }

#else

    const int idx = threadIdx.x;

    __shared__ float3 colorSum[16];
    colorSum[idx] = colors[idx];

    // Unrolled parallel reduction.
    if (idx < 8) {
        colorSum[idx] += colorSum[idx + 8];
        colorSum[idx] += colorSum[idx + 4];
        colorSum[idx] += colorSum[idx + 2];
        colorSum[idx] += colorSum[idx + 1];
    }

    // @@ Eliminate two-way bank conflicts here.
    // @@ It seems that doing that and unrolling the reduction doesn't help...
    __shared__ float covariance[16*6];
    colorSum[idx] = colors[idx] - colorSum[0] / 16.0f;
    
    covariance[6 * idx + 0] = colorSum[idx].x * colorSum[idx].x;    // 0, 6, 12, 2, 8, 14, 4, 10, 0
    covariance[6 * idx + 1] = colorSum[idx].x * colorSum[idx].y;
    covariance[6 * idx + 2] = colorSum[idx].x * colorSum[idx].z;
    covariance[6 * idx + 3] = colorSum[idx].y * colorSum[idx].y;
    covariance[6 * idx + 4] = colorSum[idx].y * colorSum[idx].z;
    covariance[6 * idx + 5] = colorSum[idx].z * colorSum[idx].z;

    for(int d = 8; d > 0; d >>= 1)
    {
        if (idx < d)
        {
            covariance[6 * idx + 0] += covariance[6 * (idx+d) + 0];
            covariance[6 * idx + 1] += covariance[6 * (idx+d) + 1];
            covariance[6 * idx + 2] += covariance[6 * (idx+d) + 2];
            covariance[6 * idx + 3] += covariance[6 * (idx+d) + 3];
            covariance[6 * idx + 4] += covariance[6 * (idx+d) + 4];
            covariance[6 * idx + 5] += covariance[6 * (idx+d) + 5];
        }
    }

#endif
    
    // Compute first eigen vector.
    return firstEigenVector(covariance);
}


#endif // _RTC_CUDAMATH_H_
