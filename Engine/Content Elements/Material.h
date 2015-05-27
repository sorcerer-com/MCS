// Mesh.h
#pragma once

#include "ContentElement.h"
#include "..\Utils\Header.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

	class ContentElement;
	using ContentElementPtr = shared_ptr < ContentElement >;

	class Material : public ContentElement
	{
	public:
		Color4 AmbientColor;
		Color4 DiffuseColor;
		Color4 SpecularColor;

		float Shininess;
		float Glossiness;

		uint TextureID;
		uint BumpmapID;
		// TODO: reflection/refraction (map, may be unite all maps in struct), IOR, Absorption	* update save/load to/from file

	public:
		Material(ContentManager* owner, const string& name, const string& package, const string& path);
		Material(ContentManager* owner, istream& file);

		ContentElementPtr GetTexture() const;
		ContentElementPtr GetBumpmap() const;

		virtual long long Size() const override;
		virtual void WriteToFile(ostream& file) override;
		virtual ContentElement* Clone() const override;

	private:
		void init();
	};

}

