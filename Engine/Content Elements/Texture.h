// Texture.h
#pragma once

#include "ContentElement.h"
#include "..\Utils\Header.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

	class Texture : public ContentElement
	{
	public:
		uint Width;                         //* group["Image"] nosave readonly
		uint Height;                        //* group["Image"] nosave readonly
		byte* Pixels;                       //* nosave

		bool Changed;                       //* default[true] nosave noproperty

	public:
		Texture(ContentManager* owner, const string& name, const string& package, const string& path);
		Texture(ContentManager* owner, istream& file);
        Texture(const Texture& texture);
        ~Texture();

        void Init(uint width, uint height);
		Color4 GetColor(uint x, uint y) const;
		Color4 GetColor(float u, float v) const;
		void SetColor(uint x, uint y, const Color4& color);
        void SetBGRAData(const byte* data);

		virtual long long Size() const override;
		virtual void WriteToFile(ostream& file) override;
		virtual ContentElement* Clone() const override;

    protected:
        int rawDataSize;
        byte* rawData;

        void init();
        void updateRawData();
        virtual void* get(const string& name);
	};

}

