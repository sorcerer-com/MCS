// Vector3.h
#pragma once

#include <math.h>


namespace MyEngine {

	struct Vector3
	{
		float x, y, z;

		Vector3() { set(0, 0, 0); }
		Vector3(const Vector3& v) { set(v.x, v.y, v.z); }
		Vector3(double _x, double _y, double _z) { set(_x, _y, _z); }

		void set(double _x, double _y, double _z)
		{
			x = (float)_x;
			y = (float)_y;
			z = (float)_z;
		}

		void makeZero(void)
		{
			x = y = z = 0.0f;
		}

		inline float length(void) const
		{
			return sqrtf(x * x + y * y + z * z);
		}

		inline float lengthSqr(void) const
		{
			return (x * x + y * y + z * z);
		}

		void scale(float multiplier)
		{
			x *= multiplier;
			y *= multiplier;
			z *= multiplier;
		}

		inline void normalize(void)
		{
			float multiplier = length();
			if (multiplier == 0.0f) return;
			multiplier = 1.0f / multiplier;
			scale(multiplier);
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

		inline void operator +=(const Vector3& v)
		{
			x += v.x;
			y += v.y;
			z += v.z;
		}
	};

	inline Vector3 operator -(const Vector3& a)
	{
		return Vector3(-a.x, -a.y, -a.z);
	}

	inline Vector3 operator +(const Vector3& a, const Vector3& b)
	{
		return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
	}

	inline Vector3 operator -(const Vector3& a, const Vector3& b)
	{
		return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	inline Vector3 operator *(const Vector3& a, const Vector3& b)
	{
		return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
	}

	inline Vector3 operator *(const Vector3& a, float m)
	{
		return Vector3(a.x * m, a.y * m, a.z * m);
	}

	inline Vector3 operator *(float m, const Vector3& a)
	{
		return Vector3(a.x * m, a.y * m, a.z * m);
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

	inline bool operator >(const Vector3& a, const Vector3& b)
	{
		return a.x > b.x && a.y > b.y && a.z > b.z;
	}


	inline float dot(const Vector3& a, const Vector3& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	inline Vector3 cross(const Vector3& a, const Vector3& b)
	{
		return Vector3(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x);
	}


	inline Vector3 barycentric(const Vector3& a, const Vector3& b, const Vector3& c, float u, float v)
	{
		return a + (b - a) * u + (c - a) * v;
	}

	inline Vector3 min(const Vector3& a, const Vector3& b)
	{
		Vector3 c;
		c.x = a.x < b.x ? a.x : b.x;
		c.y = a.y < b.y ? a.y : b.y;
		c.z = a.z < b.z ? a.z : b.z;
		return c;
	}

	inline Vector3 max(const Vector3& a, const Vector3& b)
	{
		Vector3 c;
		c.x = a.x > b.x ? a.x : b.x;
		c.y = a.y > b.y ? a.y : b.y;
		c.z = a.z > b.z ? a.z : b.z;
		return c;
	}


	inline Vector3 reflect(const Vector3& ray, const Vector3& norm)
	{
		Vector3 result = ray - dot(ray, norm) * norm * 2.0f;
		result.normalize();
		return result;
	}

	inline Vector3 faceforward(const Vector3& ray, const Vector3& norm)
	{
		if (dot(ray, norm) < 0.0f) return norm;
		else return -norm;
	}

	inline Vector3 refract(const Vector3& i, const Vector3& n, float ior)
	{
		float NdotI = dot(i, n);
		float k = 1 - (ior * ior) * (1 - NdotI * NdotI);
		if (k < 0)
			return Vector3(0, 0, 0);
		return ior * i - (ior * NdotI + (float)sqrtf(k)) * n;
	}

	inline void orthonormedSystem(const Vector3& a, Vector3& b, Vector3& c)
	{
		Vector3 temp = Vector3(1, 0, 0);
		if (abs(dot(a, temp)) > 0.99f) {
			temp = Vector3(0, 1, 0);
			if (abs(dot(a, temp)) > 0.99f)
				temp = Vector3(0, 0, 1);
		}
		b = cross(a, temp);
		b.normalize();
		c = cross(a, b);
		c.normalize();
	}
}