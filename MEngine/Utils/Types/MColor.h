// MColor.h
#pragma once

#include "Engine\Utils\Types\Color4.h"
#pragma managed

using namespace System;


namespace MyEngine {

	public value struct MColor
	{
	public:
		property double R;
		property double G;
		property double B;
		property double A;

	public:
		MColor(double _r, double _g, double _b) { Set(_r, _g, _b, 1.0); }
		MColor(double _r, double _g, double _b, double _a) { Set(_r, _g, _b, _a); }
		MColor(const Color4& c) { Set(c.r, c.g, c.b, c.a); }

		void Set(double _r, double _g, double _b, double _a)
		{
			R = _r;
			G = _g;
			B = _b;
			A = _a;
		}

		double Intensity()
		{
			return (R + G + B) / 3;
		}

		Color4 ToColor4()
		{
			return Color4(R, G, B, A);
		}

		virtual String^ ToString() override
		{
			return "(" + (float)this->R + ", " + (float)this->G + ", " + (float)this->B + ", " + (float)this->A + ")";
		}


		static MColor Parse(String^ s)
		{
			array<String^>^ separators = { "(", ")", ",", " " };
			array<String^>^ tokens = s->Split(separators, System::StringSplitOptions::RemoveEmptyEntries);
			if (tokens->Length != 4)
				return MColor();

			MColor res;
			res.R = double::Parse(tokens[0]);
			res.G = double::Parse(tokens[1]);
			res.B = double::Parse(tokens[2]);
			res.A = double::Parse(tokens[3]);

			return res;
		}


		static MColor operator -(MColor c)
		{
			return MColor(-c.R, -c.G, -c.B, -c.A);
		}

		static MColor operator +(MColor c1, MColor c2)
		{
			return MColor(c1.R + c2.R, c1.G + c2.G, c1.B + c2.B, c1.A + c2.A);
		}

		static MColor operator -(MColor c1, MColor c2)
		{
			return MColor(c1.R - c2.R, c1.G - c2.G, c1.B - c2.B, c1.A - c2.A);
		}

		static MColor operator *(MColor c1, MColor c2)
		{
			return MColor(c1.R * c2.R, c1.G * c2.G, c1.B * c2.B, c1.A * c2.A);
		}

		static MColor operator /(MColor c1, MColor c2)
		{
			return MColor(c1.R / c2.R, c1.G / c2.G, c1.B / c2.B, c1.A / c2.A);
		}

		static MColor operator *(MColor c1, double d)
		{
			return MColor(c1.R * d, c1.G * d, c1.B * d, c1.A * d);
		}

	};

}