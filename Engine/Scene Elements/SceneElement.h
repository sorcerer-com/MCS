// SceneElement.h
#pragma once

#include "..\Utils\Header.h"
#include "..\Utils\Types\Vector3.h"
#include "..\Utils\Types\Quaternion.h"

namespace MyEngine {

	class SceneManager;

	enum SceneElementType
	{
		ECamera,
		ELight,
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
		uint ContentID;
		uint MaterialID;
		// TODO: textures?
		bool Visible;
		Vector3 Position;
		Quaternion Rotation;
		Vector3 Scale;

		SceneManager* Owner;

	public:
		SceneElement(SceneManager* owner, SceneElementType type, const string& name, uint contentID);
		SceneElement(SceneManager* owner, istream& file);

		virtual void WriteToFile(ostream& file) const;
		virtual SceneElement* Clone() const;
	};

}

