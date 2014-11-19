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
	};

}