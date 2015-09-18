// MEngine.cpp

#include "stdafx.h"
#include "MEngine.h"


namespace MyEngine {

    EEngineMode MEngine::Mode::get()
    {
        return (EEngineMode)Engine::Mode;
    }

    void MEngine::Mode::set(EEngineMode value)
    {
        Engine::Mode = (EngineMode)value;
    }


	MEngine::MEngine()
	{
		this->engine = new Engine();

		this->ContentManager = gcnew MContentManager(this, this->engine->ContentManager.get());
        this->SceneManager = gcnew MSceneManager(this, this->engine->SceneManager.get());
        this->AnimationManager = gcnew MAnimationManager(this, this->engine->AnimationManager.get());

        this->ViewPortRenderer = gcnew MViewPortRenderer(this->engine->ViewPortRenderer.get());
        this->ProductionRenderer = gcnew MProductionRenderer(this->engine->ProductionRenderer.get());
	}

	MEngine::~MEngine()
    {
        delete this->ProductionRenderer;
        this->ProductionRenderer = nullptr;

        delete this->ViewPortRenderer;
        this->ViewPortRenderer = nullptr;

        delete this->AnimationManager;
        this->AnimationManager = nullptr;

        delete this->SceneManager;
        this->SceneManager = nullptr;

		delete this->ContentManager;
		this->ContentManager = nullptr;

		delete this->engine;
		this->engine = NULL;
	}


	void MEngine::Log(ELogType type, String^ category, String^ text)
	{
		Engine::Log((LogType)type, to_string(category), to_string(text));
	}

}