// Mesh.h
#pragma once

#include "ContentElement.h"
#include "..\Utils\Header.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

	class ContentElement;
	using ContentElementPtr = shared_ptr < ContentElement >;

    struct TextureSet
    {
        uint DiffuseMapID; // alpha - refraction
        uint NormalMapID; // alpha - reflection
    };

	class Material : public ContentElement
	{
	public:
		Color4 DiffuseColor;                //* default[Color4(0.8f, 0.8f, 0.8f, 1.0f)] group["Colors"]     // alpha - refraction
		Color4 SpecularColor;               //* default[Color4(0.0f, 0.0f, 0.0f, 1.0f)] group["Colors"] 	// alpha - reflection
        Color4 InnerColor;                  //* default[Color4(0.0f, 0.0f, 0.0f, 1.0f)] group["Colors"]

		float Shininess;                    //* default[10.0f] group["Material"]
		float Glossiness;                   //* default[1.0f] group["Material"]
        float IOR;                          //* default[1.5f] group["Material"]
        float Absorption;                   //* default[0.1f] group["Material"]

        TextureSet Textures;                //* noinit noproperty

	public:
		Material(ContentManager* owner, const string& name, const string& package, const string& path);
		Material(ContentManager* owner, istream& file);

        ContentElementPtr GetDiffuseMap() const;
        ContentElementPtr GetNormalMap() const;

		virtual long long Size() const override;
		virtual void WriteToFile(ostream& file) override;
		virtual ContentElement* Clone() const override;

    protected:
        void init();
        virtual void* get(const string& name);
	};

}

