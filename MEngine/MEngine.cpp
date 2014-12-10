// MEngine.cpp

#include "stdafx.h"
#include "MEngine.h"


namespace MyEngine {

	MEngine::MEngine()
	{
		this->engine = new Engine();

		this->ContentManager = gcnew MContentManager(this->engine->ContentManager.get());
		this->SceneManagaer = gcnew MSceneManager(this->engine->SceneManager.get());
	}

	MEngine::~MEngine()
	{
		delete this->SceneManagaer;
		this->SceneManagaer = nullptr;

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