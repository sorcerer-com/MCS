// MScene.h
#pragma once

#include "Scene.h"
#pragma managed

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

		static EEngineMode Mode;

	public:
		MScene();
		~MScene();
	};

}