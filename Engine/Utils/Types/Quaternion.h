// Quaternion.h
#pragma once


namespace MyEngine {

	struct Quaternion
	{
		float x, y, z, w;

		Quaternion() { set(0, 0, 0, 1); }
		Quaternion(const Quaternion& q) { set(q.x, q.y, q.z, q.w); }
		Quaternion(double _x, double _y, double _z, double _w) { set(_x, _y, _z, _w); }
		Quaternion(Vector3 axisAngle)
		{
			float angle = axisAngle.length();
			axisAngle.normalize();
			float sin = sinf(angle / 2.0f);
			float cos = cosf(angle / 2.0f);
			set(axisAngle.x * sin, axisAngle.y * sin, axisAngle.z * sin, cos);
		}

		void set(double _x, double _y, double _z, double _w)
		{
			x = (float)_x;
			y = (float)_y;
			z = (float)_z;
			w = (float)_w;
		}

		void makeZero(void)
		{
			x = y = z = 0.0;
			w = 1.0f;
		}

		inline float magnitude(void) const
		{
			return sqrtf(x * x + y * y + z * z + w * w);
		}

		inline float magnitudeSqr(void) const
		{
			return (x * x + y * y + z * z + w * w);
		}

		void scale(float multiplier)
		{
			x *= multiplier;
			y *= multiplier;
			z *= multiplier;
			w *= multiplier;
		}

		inline void normalize(void)
		{
			float multiplier = magnitude();
			if (multiplier == 0.0f) return;
			multiplier = 1.0f / multiplier;
			scale(multiplier);
		}

		Vector3 getAxisAngle(void) const
		{
			float angle = 2 * acosf(w);
			Vector3 res(x, y, z);
			res *= 1.0f / sqrtf(1 - w * w);
			res *= angle;
			return res;
		}

		inline void operator +=(const Quaternion& q)
		{
			x += q.x;
			y += q.y;
			z += q.z;
			w += q.w;
		}
	};
	

	inline Quaternion operator +(const Quaternion& a, const Quaternion& b)
	{
		return Quaternion(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
	}

	inline Quaternion operator -(const Quaternion& a, const Quaternion& b)
	{
		return Quaternion(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
	}

	inline Quaternion operator *(const Quaternion& a, const Quaternion& b)
	{
		Quaternion q;
		q.x = a.x * b.x - a.y * b.y - a.z * b.z - a.w * b.w;
		q.y = a.x * b.y + a.y * b.x + a.z * b.w - a.w * b.z;
		q.z = a.x * b.z - a.y * b.w + a.z * b.x + a.w * b.y;
		q.w = a.x * b.w + a.y * b.z - a.z * b.y + a.w * b.x;
		return q;
	}

}