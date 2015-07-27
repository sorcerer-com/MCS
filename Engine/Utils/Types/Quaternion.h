// Quaternion.h
#pragma once


namespace MyEngine {

	struct Quaternion
	{
		float x, y, z, w;

		Quaternion() { set(0, 0, 0, 1); }
		Quaternion(const Quaternion& q) { set(q.x, q.y, q.z, q.w); }
		Quaternion(double _x, double _y, double _z, double _w) { set(_x, _y, _z, _w); }
		Quaternion(Vector3 eulerAngle)
		{
            eulerAngle *= 3.14159265359f / 180.0f; // from deg to rad

            float angle = eulerAngle.x * 0.5f;
            const float sr = sinf(angle);
            const float cr = cosf(angle);

            angle = eulerAngle.y * 0.5f;
            const float sp = sinf(angle);
            const float cp = cosf(angle);

            angle = eulerAngle.z * 0.5f;
            const float sy = sinf(angle);
            const float cy = cosf(angle);

            const float cpcy = cp * cy;
            const float spcy = sp * cy;
            const float cpsy = cp * sy;
            const float spsy = sp * sy;

            x = sr * cpcy - cr * spsy;
            y = cr * spcy + sr * cpsy;
            z = cr * cpsy - sr * spcy;
            w = cr * cpcy + sr * spsy;

			this->normalize();
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
			return sqrt(x * x + y * y + z * z + w * w);
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

		Vector3 toEulerAngle(void) const
		{
			float sqw = w * w;
			float sqx = x * x;
			float sqy = y * y;
			float sqz = z * z;
            float test = 2.0f * (y * w - x * z);

            Vector3 result;
            if (abs(test - 1.0f) < 0.000001f) // equal to 1.0 with epsilon
            {
                result.z = -2.0f * atan2f(x, w);
                result.x = 0.0f;
                result.y = 3.14159265359f / 2.0f;
            }
            else if (abs(test + 1.0f) < 0.000001f) // equal to -1.0 with epsilon
            {
                result.z = 2.0f * atan2f(x, w);
                result.x = 0.0f;
                result.y = 3.14159265359f / -2.0f;
            }
            else
            {
                result.z = atan2f(2.0f * (x * y + z * w), (sqx - sqy - sqz + sqw));
                result.x = atan2f(2.0f * (y * z + x * w), (-sqx - sqy + sqz + sqw));
                result.y = asinf(std::min(std::max(test, -1.0f), 1.0f));
            }

            result *= 180.0f / 3.14159265359f; // from rad to deg
			return result;
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
        q.w = (b.w * a.w) - (b.x * a.x) - (b.y * a.y) - (b.z * a.z);
        q.x = (b.w * a.x) + (b.x * a.w) + (b.y * a.z) - (b.z * a.y);
        q.y = (b.w * a.y) + (b.y * a.w) + (b.z * a.x) - (b.x * a.z);
        q.z = (b.w * a.z) + (b.z * a.w) + (b.x * a.y) - (b.y * a.x);
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