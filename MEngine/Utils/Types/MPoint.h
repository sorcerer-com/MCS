// MPoint.h
#pragma once

#include "Engine\Utils\Types\Vector3.h"
#include "Engine\Utils\Types\Quaternion.h"
#pragma managed

using namespace System;


namespace MyEngine {

	public value struct MPoint
	{
	public:
		property double X;
		property double Y;
		property double Z;

	public:
		MPoint(double _x, double _y, double _z) { Set(_x, _y, _z); }
		MPoint(const Vector3& v) { Set(v.x, v.y, v.z); }

		void Set(double _x, double _y, double _z)
		{
			X = _x;
			Y = _y;
			Z = _z;
		}

		Vector3 ToVector3()
		{
			return Vector3(X, Y, Z);
		}

		virtual String^ ToString() override
		{
			return "(" + (float)this->X + ", " + (float)this->Y + ", " + (float)this->Z + ")";
		}

		double Length()
		{
			return sqrt(X * X + Y * Y + Z * Z);
		}

		void Normalize()
		{
			double m = 1.0 / Length();
			if (m == 0.0) return;
			Set(X * m, Y * m, Z * m);
		}

		void RotateBy(MPoint angles)
		{
			Vector3 result = Quaternion(angles.ToVector3()) * this->ToVector3();
			this->Set(result.x, result.y, result.z);
		}

		static MPoint operator -(MPoint p)
		{
			return MPoint(-p.X, -p.Y, -p.Z);
		}

		static MPoint operator +(MPoint p1, MPoint p2)
		{
			return MPoint(p1.X + p2.X, p1.Y + p2.Y, p1.Z + p2.Z);
		}

		static MPoint operator -(MPoint p1, MPoint p2)
		{
			return MPoint(p1.X - p2.X, p1.Y - p2.Y, p1.Z - p2.Z);
		}

		static MPoint operator *(MPoint p1, MPoint p2)
		{
			return MPoint(p1.X * p2.X, p1.Y * p2.Y, p1.Z * p2.Z);
		}

		static MPoint operator /(MPoint p1, MPoint p2)
		{
			return MPoint(p1.X / p2.X, p1.Y / p2.Y, p1.Z / p2.Z);
		}

		static MPoint operator *(MPoint p1, double d)
		{
			return MPoint(p1.X * d, p1.Y * d, p1.Z * d);
		}

		static MPoint Min(MPoint a, MPoint b)
		{
			MPoint p;
			p.X = a.X < b.X ? a.X : b.X;
			p.Y = a.Y < b.Y ? a.Y : b.Y;
			p.Z = a.Z < b.Z ? a.Z : b.Z;
			return p;
		}

		static MPoint Max(MPoint a, MPoint b)
		{
			MPoint p;
			p.X = a.X > b.X ? a.X : b.X;
			p.Y = a.Y > b.Y ? a.Y : b.Y;
			p.Z = a.Z > b.Z ? a.Z : b.Z;
			return p;
		}

	};
}