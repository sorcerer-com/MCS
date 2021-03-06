// MEngine.h
#pragma once

#include "Engine\Engine.h"
#pragma managed

#include "Managers\MContentManager.h"
#include "Managers\MSceneManager.h"
#include "Managers\MAnimationManager.h"
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
        property MAnimationManager^ AnimationManager;

        property MViewPortRenderer^ ViewPortRenderer;
        property MProductionRenderer^ ProductionRenderer;

        property bool IsStarted
        {
            bool get();
            void set(bool value);
        }


        static property EEngineMode Mode
        {
            EEngineMode get();
            void set(EEngineMode value);
        }

        static property Dictionary<String^, TimeSpan>^ ProfilerData
        {
            Dictionary<String^, TimeSpan>^ get();
        }


        delegate void StartedEventHandler(MEngine^ sender, bool value);
        event StartedEventHandler^ StartChanging;
        event StartedEventHandler^ StartChanged;

	public:
		MEngine();
		~MEngine();

        void Start();
        void Stop();

		static void Log(ELogType type, String^ category, String^ text);

    private:
        void OnStartChanging(bool value);
        void OnStartChanged(bool value);

	};

}