// MUtil.h
#pragma once

#include "Engine\Utils\Types\Vector3.h"
#pragma managed

using namespace Engine;
using namespace System;


namespace MEngine {

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

		/* TODO: there isn't Matrix yet
		MPoint Rotate(MPoint angle, MPoint center)
		{
			Matrix3 ma3x(1.0f);
			ma3x = Matrix3::makeRotationMatrix(AXIS_Y, (float)DegToRad(angle.Y)) *
				Matrix3::makeRotationMatrix(AXIS_X, (float)DegToRad(angle.X)) *
				Matrix3::makeRotationMatrix(AXIS_Z, (float)DegToRad(angle.Z)) * ma3x;
			Vector3 vec = (*this - center).ToVector3();
			vec = ma3x * vec;
			return MPoint(vec) + center;
		} */

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

		static double DegToRad(double angle)
		{
			const double PI = 3.14159265;
			return -PI * angle / 180.0;
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