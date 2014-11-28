// MScene.h
#pragma once

#include "Engine\Scene.h"
#pragma managed

#include "Managers\MContentManager.h"

using namespace Engine;


namespace MEngine {

	public enum class EEngineMode
	{
		Editor,
		Engine
	};

	public enum class ELogType
	{
		Log,
		Warning,
		Error
	};

	public ref class MScene
	{
	private:
		Scene* scene;

	public:
		property MContentManager^ ContentManager;

		static EEngineMode Mode;

	public:
		MScene();
		~MScene();

		static void Log(ELogType type, String^ category, String^ text);
	};

}