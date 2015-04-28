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

		this->ContentManager = gcnew MContentManager(this->engine->ContentManager.get());
		this->SceneManager = gcnew MSceneManager(this->engine->SceneManager.get());

		this->ViewPortRenderer = gcnew MRenderer(this->engine->ViewPortRenderer.get());
	}

	MEngine::~MEngine()
	{
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