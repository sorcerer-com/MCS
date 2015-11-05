// Color4.h
#pragma once

#include <math.h>


namespace MyEngine {

#ifdef __SSE__
#pragma warning( disable : 4793 )
#pragma optimize( "g", off )
#endif

	struct Color4
	{
		float r, g, b, a;

		Color4() { set(0, 0, 0, 0); }
		Color4(const Color4& c) { set(c.r, c.g, c.b, c.a); }
		Color4(double _r, double _g, double _b) { set(_r, _g, _b, 1.0); }
        Color4(double _r, double _g, double _b, double _a) { set(_r, _g, _b, _a); }
#ifdef __SSE__
        Color4(const __m128& xmm_) { set(xmm_.m128_f32[0], xmm_.m128_f32[1], xmm_.m128_f32[2], xmm_.m128_f32[3]); }
#endif

		void set(double _r, double _g, double _b, double _a)
		{
			r = (float)_r;
			g = (float)_g;
			b = (float)_b;
			a = (float)_a;
		}

		void makeZero(void)
		{
			r = g = b = 0.0f;
			a = 1.0f;
		}

		void makeOne(void)
		{
			r = g = b = a = 1.0f;
        }

        inline const float& operator[](int idx) const
        {
            return ((idx == 0) ? r : ((idx == 1) ? g : ((idx == 2) ? b : a)));
        }

        inline float& operator[](int idx)
        {
            return ((idx == 0) ? r : ((idx == 1) ? g : ((idx == 2) ? b : a)));
        }

		inline void operator +=(const Color4& c)
        {
#ifdef __SSE__
            *this = _mm_add_ps(*this, c);
#else
			r += c.r;
			g += c.g;
			b += c.b;
            a += c.a;
#endif
		}

		inline void operator -=(const Color4& c)
        {
#ifdef __SSE__
            *this = _mm_sub_ps(*this, c);
#else
			r -= c.r;
			g -= c.g;
			b -= c.b;
            a -= c.a;
#endif
		}

		inline void operator *=(const Color4& c)
        {
#ifdef __SSE__
            *this = _mm_mul_ps(*this, c);
#else
			r *= c.r;
			g *= c.g;
			b *= c.b;
            a *= c.a;
#endif
		}

		inline void operator *=(double multiplier)
        {
#ifdef __SSE__
            Color4 c(multiplier, multiplier, multiplier, multiplier);
            *this = _mm_mul_ps(*this, c);
#else
			r *= (float)multiplier;
			g *= (float)multiplier;
			b *= (float)multiplier;
            a *= (float)multiplier;
#endif
		}

		inline float intensity() const
		{
			return (this->r + this->g + this->b) / 3;
		}

        inline static Color4 Black()
        {
            return Color4(0.0f, 0.0f, 0.0f);
        }

        inline static Color4 White()
        {
            return Color4(1.0f, 1.0f, 1.0f);
        }


#ifdef __SSE__
        inline operator __m128() const
        {
            return *((__m128*)this);
        }
#endif
	};

	inline Color4 operator *(const Color4& a, float f)
    {
#ifdef __SSE__
        Color4 c(f, f, f, f);
        return _mm_mul_ps(a, c);
#else
        return Color4(a.r * f, a.g * f, a.b * f, a.a * f);
#endif
    }

    inline Color4 operator *(const Color4& a, const Color4& b)
    {
#ifdef __SSE__
        return _mm_mul_ps(a, b);
#else
        return Color4(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a);
#endif
    }

    inline Color4 operator +(const Color4& a, const Color4& b)
    {
#ifdef __SSE__
        return _mm_add_ps(a, b);
#else
        return Color4(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
#endif
    }

    inline Color4 operator -(const Color4& a, const Color4& b)
    {
#ifdef __SSE__
        return _mm_sub_ps(a, b);
#else
        return Color4(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a);
#endif
    }


    inline bool operator ==(const Color4& p1, const Color4& p2)
    {
        return p1.r == p2.r && p1.g == p2.g && p1.b == p2.b && p1.a == p2.a;
    }

    inline bool operator !=(const Color4& p1, const Color4& p2)
    {
        return !(p1 == p2);
    }
    
    inline bool operator <(const Color4& p1, float f)
    {
        return p1.r < f && p1.g < f && p1.b < f && p1.a < f;
    }


    inline Color4 linearFilter(const Color4& a, const Color4& b, const Color4& c, float u, float v)
    {
        return a + (b - a) * u + (c - a) * v;
    }

	inline Color4 linearFilter(const Color4& a, const Color4& b, const Color4& c, const Color4& d, float u, float v)
	{
		return a * (1 - u) * (1 - v) + b * u * (1 - v) + c * (1 - u) * v + d * u * v;
	}

    inline Color4 absolute(const Color4& c)
    {
        return Color4(abs(c.r), abs(c.g), abs(c.b), abs(c.a));
    }

#ifdef __SSE__
#pragma warning( default : 4793 )
#pragma optimize( "g", on )
#endif

}