// Texture.h
#pragma once

#include "ContentElement.h"
#include "..\Utils\Header.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

	class Texture : public ContentElement
	{
	public:
		uint Width;
		uint Height;
		// TODO: bool changed;?
		byte* Pixels;
		int rawDataSize;

	public:
		Texture(ContentManager* owner, const string& name, const string& package, const string& path);
		Texture(ContentManager* owner, istream& file);

		void Init(uint width, uint height);
		Color4 GetColor(uint x, uint y) const;
		Color4 GetColor(float u, float v) const;
		void SetColor(uint x, uint y, const Color4& color);

		virtual long long Size() const override;
		virtual void WriteToFile(ostream& file) override;
		virtual ContentElement* Clone() const override;

	private:
		void init();
	};

}

