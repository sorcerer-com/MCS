// Vector3.h
#pragma once

#include <math.h>

namespace MyEngine {

#ifdef __SSE__
#pragma warning( disable : 4793 )
#pragma optimize( "g", off )
#endif
    
    struct Vector3
	{
        float x, y, z, w;

		Vector3() { set(0, 0, 0); }
        Vector3(const Vector3& v) { set(v.x, v.y, v.z); }
        Vector3(double _x, double _y, double _z) { set(_x, _y, _z); }
#ifdef __SSE__
        Vector3(const __m128& xmm_) { set(xmm_.m128_f32[0], xmm_.m128_f32[1], xmm_.m128_f32[2]); }
#endif

		void set(double _x, double _y, double _z)
		{
			x = (float)_x;
			y = (float)_y;
			z = (float)_z;
            w = 0.0f;
		}

		void makeZero(void)
		{
			x = y = z = 0.0f;
		}

		inline float length(void) const
		{
            return sqrt(lengthSqr());
		}

		inline float lengthSqr(void) const
		{
#ifdef __SSE__
            return _mm_dp_ps(*this, *this, 0xFF).m128_f32[0];
#else
			return (x * x + y * y + z * z);
#endif
		}

		void scale(float multiplier)
        {
#ifdef __SSE__
            Vector3 v(multiplier, multiplier, multiplier);
            *this = _mm_mul_ps(*this, v);
#else
			x *= multiplier;
			y *= multiplier;
            z *= multiplier;
#endif
		}

		inline void normalize(void)
        {
#ifdef __SSE__
            const auto& dot = _mm_dp_ps(*this, *this, 0xFF);
            if (dot.m128_f32[0] == 0.0f) return;
            const auto& invLength = _mm_rsqrt_ps(dot);
            *this = _mm_mul_ps(*this, invLength);
#else
			float multiplier = length();
			if (multiplier == 0.0f) return;
			multiplier = 1.0f / multiplier;
            scale(multiplier);
#endif
		}

		inline int maxDim() const
		{
			int bi = 0;
			float maxD = abs(x);
			if (abs(y) > maxD) { maxD = abs(y); bi = 1; }
			if (abs(z) > maxD) { maxD = abs(z); bi = 2; }
			return bi;
		}

		inline const float& operator[](int idx) const
		{
			return ((idx == 0) ? x : ((idx == 1) ? y : z));
		}

		inline float& operator[](int idx)
		{
			return ((idx == 0) ? x : ((idx == 1) ? y : z));
		}

		inline void operator *=(float multiplier)
		{
			scale(multiplier);
        }

        inline void operator *=(const Vector3& v)
        {
#ifdef __SSE__
            *this = _mm_mul_ps(*this, v);
#else
            x *= v.x;
            y *= v.y;
            z *= v.z;
#endif
        }

		inline void operator +=(const Vector3& v)
        {
#ifdef __SSE__
            *this = _mm_add_ps(*this, v);
#else
			x += v.x;
			y += v.y;
            z += v.z;
#endif
		}


#ifdef __SSE__
        inline operator __m128() const
        {
            return *((__m128*)this);
        }
#endif
	};

	inline Vector3 operator -(const Vector3& a)
	{
		return Vector3(-a.x, -a.y, -a.z);
	}

	inline Vector3 operator +(const Vector3& a, const Vector3& b)
    {
#ifdef __SSE__
        return _mm_add_ps(a, b);
#else
		return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
#endif
	}

	inline Vector3 operator -(const Vector3& a, const Vector3& b)
    {
#ifdef __SSE__
        return _mm_sub_ps(a, b);
#else
		return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
#endif
	}

	inline Vector3 operator *(const Vector3& a, const Vector3& b)
    {
#ifdef __SSE__
        return _mm_mul_ps(a, b);
#else
		return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
#endif
	}

	inline Vector3 operator *(const Vector3& a, float m)
    {
#ifdef __SSE__
        Vector3 v(m, m, m);
        return _mm_mul_ps(a, v);
#else
		return Vector3(a.x * m, a.y * m, a.z * m);
#endif
	}

	inline Vector3 operator *(float m, const Vector3& a)
    {
#ifdef __SSE__
        Vector3 v(m, m, m);
        return _mm_mul_ps(a, v);
#else
		return Vector3(a.x * m, a.y * m, a.z * m);
#endif
	}


	inline bool operator ==(const Vector3& a, const Vector3& b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	inline bool operator !=(const Vector3& a, const Vector3& b)
	{
		return !(a == b);
	}

	inline bool operator <(const Vector3& a, const Vector3& b)
	{
		return a.x < b.x && a.y < b.y && a.z < b.z;
    }

    inline bool operator <=(const Vector3& a, const Vector3& b)
    {
        return a.x <= b.x && a.y <= b.y && a.z <= b.z;
    }

	inline bool operator >(const Vector3& a, const Vector3& b)
	{
		return a.x > b.x && a.y > b.y && a.z > b.z;
	}

    inline bool operator >=(const Vector3& a, const Vector3& b)
    {
        return a.x >= b.x && a.y >= b.y && a.z >= b.z;
    }


	inline float dot(const Vector3& a, const Vector3& b)
    {
#ifdef __SSE__
        return _mm_dp_ps(a, b, 0xFF).m128_f32[0];
#else
		return a.x * b.x + a.y * b.y + a.z * b.z;
#endif
	}

	inline Vector3 cross(const Vector3& a, const Vector3& b)
    {
#ifdef __SSE__
        __m128 v0;
        __m128 v1;
        __m128 v2;
        __m128 v3;
        v0 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2));    // v0 = (z, x, y, w)
        v1 = _mm_mul_ps(v0, b);
        v2 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2));    // v2 = (z, x, y, w)
        v3 = _mm_mul_ps(v2, a);
        v3 = _mm_sub_ps(v1, v3);
        return(_mm_shuffle_ps(v3, v3, _MM_SHUFFLE(3, 1, 0, 2)));
#else
		return Vector3(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x);
#endif
	}


	inline Vector3 barycentric(const Vector3& a, const Vector3& b, const Vector3& c, float u, float v)
	{
		return a + (b - a) * u + (c - a) * v;
	}

#ifdef __SSE__
#pragma warning( default : 4793 )
#pragma optimize( "g", on )
#endif

}