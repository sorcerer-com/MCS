// SceneManager.h
#pragma once

#include "..\Utils\Header.h"


namespace MyEngine {

	class Engine;
	class SceneElement;
	enum SceneElementType;

	using SceneElementPtr = shared_ptr < SceneElement >;

	class SceneManager
	{
	public:
		using SceneMapType = map < uint, SceneElementPtr >; // id / scene element

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
		bool DeleteElement(uint id);
		SceneElementPtr GetElement(uint id);
		SceneElementPtr GetElement(const string& name);

	};

}