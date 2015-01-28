// SceneManager.h
#pragma once

#include "..\Utils\Header.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

	class Engine;
	enum SceneElementType;
	class SceneElement;
	class Camera;

	using SceneElementPtr = shared_ptr < SceneElement >;

	class SceneManager
	{
	public:
		using SceneMapType = map < uint, SceneElementPtr >; // id / scene element

		Camera* ActiveCamera;
		Color4 AmbientLight;

		Engine* Owner;

	private:
		SceneMapType sceneElements;
		// TODO: layers

	public:
		SceneManager(Engine* owner);

		void New();
		bool Save(const string& filePath);
		bool Load(const string& filePath);

		SceneElementPtr AddElement(SceneElementType type, const string& name, uint contentID, uint id = 0);
		SceneElementPtr AddElement(SceneElementType type, const string& name, const string& contentFullName, uint id = 0);
		bool AddElement(SceneElement* element);
		bool ContainElement(uint id) const;
		bool ContainElement(const string& name) const;
		bool DeleteElement(uint id);
		SceneElementPtr GetElement(uint id);
		SceneElementPtr GetElement(const string& name);
		vector<SceneElementPtr> GetElements();

	};

}