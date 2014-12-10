// MSceneManager.h
#pragma once

#include "Engine\Managers\SceneManager.h"
#pragma managed

#include "..\Utils\MHeader.h"


namespace MyEngine {

	enum class ESceneElementType;
	ref class MSceneElement;

	public ref class MSceneManager
	{
	private:
		SceneManager* sceneManager;

	public:
		MSceneManager(SceneManager* sceneManager);

		void New();
		bool Load(String^ filePath);
		bool Save(String^ filePath);

		MSceneElement^ AddElement(ESceneElementType type, String^ name, uint contentID);
		MSceneElement^ AddElement(ESceneElementType type, String^ name, String^ path);
		MSceneElement^ CloneElement(MSceneElement^ element, String^ newName);
		bool ContainElement(uint id);
		bool RenameElement(String^ oldName, String^ newName);
		bool DeleteElement(uint id);
		MSceneElement^ GetElement(uint id);
		MSceneElement^ GetElement(String^ name);

	private:
		static MSceneElement^ getMSceneElement(const SceneElementPtr& element);

	};

}