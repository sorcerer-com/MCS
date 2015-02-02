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
			angle *= 3.14159265f / 180.0f; // from deg to rad
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

		inline void conjugate(void)
		{
			x = -x;
			y = -y;
			z = -z;
			w *= -1;
		}

		Vector3 toAxisAngle(void) const
		{
			if (w == 1.0f)
				return Vector3();

			float angle = 2 * acosf(w);
			Vector3 res(x, y, z);
			res *= 1.0f / sqrtf(1 - w * w);
			res *= angle;
			res *= 180 / 3.14159265f; // from rad to deg
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

	inline Vector3 operator *(const Quaternion& q, const Vector3& v)
	{
		Vector3 result;
		/*result.x = q.w * q.w * v.x + 2 * q.y * q.w * v.z - 2 * q.z * q.w * v.y + q.x * q.x * v.x + 2 * q.y * q.x * v.y + 2 * q.z * q.x * v.z - q.z * q.z * v.x - q.y * q.y * v.x;
		result.y = 2 * q.x * q.y * v.x + q.y * q.y * v.y + 2 * q.z * q.y * v.z + 2 * q.w * q.z * v.x - q.z * q.z * v.y + q.w * q.w * v.y - 2 * q.x * q.w * v.z - q.x * q.x * v.y;
		result.z = 2 * q.x * q.z * v.x + 2 * q.y * q.z * v.y + q.z * q.z * v.z - 2 * q.w * q.y * v.x - q.y * q.y * v.z + 2 * q.w * q.x * v.y - q.x * q.x * v.z + q.w * q.w * v.z;*/
		// faster
		Vector3 xyz(q.x, q.y, q.z);
		Vector3 t = cross(xyz, v) * 2;
		result = v + q.w * t + cross(xyz, t);
		return result;
	}

}