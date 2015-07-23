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
		Color4 AmbientColor;
		Color4 DiffuseColor; // alpha - refraction
		Color4 SpecularColor; // alpha - reflection

		float Shininess;
		float Glossiness;
        float IOR;

        TextureSet Textures;
		// TODO: InnerColor, Absorption	* update save/load to/from file

	public:
		Material(ContentManager* owner, const string& name, const string& package, const string& path);
		Material(ContentManager* owner, istream& file);

        ContentElementPtr GetDiffuseMap() const;
        ContentElementPtr GetNormalMap() const;

		virtual long long Size() const override;
		virtual void WriteToFile(ostream& file) override;
		virtual ContentElement* Clone() const override;

	private:
		void init();
	};

}

