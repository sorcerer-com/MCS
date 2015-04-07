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
		MColor(System::Drawing::Color c) { Set((double)c.R / 255, (double)c.G / 255, (float)c.B / 255, (double)c.A / 255); }

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

		System::Drawing::Color ToColor()
		{
			int r = (int)(std::max(std::min(R, 1.0), 0.0) * 255);
			int g = (int)(std::max(std::min(G, 1.0), 0.0) * 255);
			int b = (int)(std::max(std::min(B, 1.0), 0.0) * 255);
			int a = (int)(std::max(std::min(A, 1.0), 0.0) * 255);
			return System::Drawing::Color::FromArgb(a, r, g, b);
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