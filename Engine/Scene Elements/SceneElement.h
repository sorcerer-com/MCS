// SceneElement.h
#pragma once

#include "..\Utils\Header.h"
#include "..\Utils\Types\Vector3.h"
#include "..\Utils\Types\Quaternion.h"
#include "..\Content Elements\Material.h"

namespace MyEngine {

	class SceneManager;

	class ContentElement;
	using ContentElementPtr = shared_ptr < ContentElement >;

	enum SceneElementType
	{
		ECamera,
        ELight,
        ESkyBox,
		ESystemObject,
		EStaticObject,
	};

	class SceneElement
	{
	public:
		uint Version;
		SceneElementType Type;
		uint ID;
		string Name;
		string Layer;
		uint ContentID;
		uint MaterialID;
        TextureSet Textures;
		// TODO: attach?
		bool Visible;
		Vector3 Position;
		Quaternion Rotation;
		Vector3 Scale;

		SceneManager* Owner;

	public:
		SceneElement(SceneManager* owner, SceneElementType type, const string& name, uint contentID);
		SceneElement(SceneManager* owner, istream& file);

		ContentElementPtr GetContent() const;
        ContentElementPtr GetMaterial() const;
        ContentElementPtr GetDiffuseMap() const;
        ContentElementPtr GetNormalMap() const;

		virtual void WriteToFile(ostream& file) const;
		virtual SceneElement* Clone() const;

	private:
		void init();
	};

}

