// MEngine.h
#pragma once

#include "Engine\Engine.h"
#pragma managed

#include "Managers\MContentManager.h"
#include "Managers\MSceneManager.h"
#include "Renderers\MViewPortRenderer.h"
#include "Renderers\MProductionRenderer.h"
#include "Utils\MSelector.h"


namespace MyEngine {

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

	public ref class MEngine
	{
	private:
		Engine* engine;

	public:
		property MContentManager^ ContentManager;
		property MSceneManager^ SceneManager;

        property MViewPortRenderer^ ViewPortRenderer;
        property MProductionRenderer^ ProductionRenderer;

        static property EEngineMode Mode
        {
            EEngineMode get();
            void set(EEngineMode value);
        }

	public:
		MEngine();
		~MEngine();

		static void Log(ELogType type, String^ category, String^ text);
	};

}