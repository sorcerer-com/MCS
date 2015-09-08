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
        SceneManager* Owner;                //* noinit

		uint Version;                       //* default[CURRENT_VERSION] nosave
		SceneElementType Type;              //* default[SceneElementType::ECamera]
		uint ID;                            //* default[INVALID_ID]
		string Name;                        //*
		string Layer;                       //* default[DEFAULT_LAYER_NAME]
		uint ContentID;                     //* default[INVALID_ID]
		uint MaterialID;                    //* default[INVALID_ID]
        TextureSet Textures;                //* noinit
		// TODO: attach?
		bool Visible;                       //* default[true]
		Vector3 Position;                   //*
		Quaternion Rotation;                //*
		Vector3 Scale;                      //* default[Vector3(1.0, 1.0, 1.0)]

	public:
		SceneElement(SceneManager* owner, SceneElementType type, const string& name, uint contentID);
		SceneElement(SceneManager* owner, istream& file);
        virtual ~SceneElement();

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

