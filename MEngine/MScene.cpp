// MScene.cpp

#include "stdafx.h"
#include "MScene.h"


namespace MEngine {

	MScene::MScene()
	{
		this->scene = new Scene();

		this->ContentManager = gcnew MContentManager(this->scene->ContentManager.get());
	}

	MScene::~MScene()
	{
		delete this->ContentManager;
		this->ContentManager = nullptr;

		delete this->scene;
		this->scene = NULL;
	}


	void MScene::Log(ELogType type, String^ category, String^ text)
	{
		Scene::Log((LogType)type, to_string(category), to_string(text));
	}

}