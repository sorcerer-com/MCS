// Color4.h
#pragma once

#include <math.h>


namespace MyEngine {

	struct Color4
	{
		float r, g, b, a;

		Color4() { set(0, 0, 0, 0); }
		Color4(const Color4& c) { set(c.r, c.g, c.b, c.a); }
		Color4(double _r, double _g, double _b) { set(_r, _g, _b, 1.0); }
		Color4(double _r, double _g, double _b, double _a) { set(_r, _g, _b, _a); }

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

		inline void operator +=(const Color4& c)
		{
			r += c.r;
			g += c.g;
			b += c.b;
			a += c.a;
		}

		inline void operator -=(const Color4& c)
		{
			r -= c.r;
			g -= c.g;
			b -= c.b;
			a -= c.a;
		}

		inline void operator *=(const Color4& c)
		{
			r *= c.r;
			g *= c.g;
			b *= c.b;
			a *= c.a;
		}

		inline void operator *=(double multiplier)
		{
			r *= (float)multiplier;
			g *= (float)multiplier;
			b *= (float)multiplier;
			a *= (float)multiplier;
		}

		inline float intensity() const
		{
			return (this->r + this->g + this->b) / 3;
		}
	};

	inline Color4 operator *(const Color4& a, float f)
	{
		return Color4(a.r * f, a.g * f, a.b * f, a.a * f);
	}

	inline Color4 operator +(const Color4& a, const Color4& b)
	{
		return Color4(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
	}


	inline Color4 linearFilter(const Color4& a, const Color4& b, const Color4& c, const Color4& d, float u, float v)
	{
		return a * (1 - u) * (1 - v) + b * u * (1 - v) + c * (1 - u) * v + d * u * v;
	}

}