// MScene.cpp

#include "stdafx.h"
#include "MScene.h"


namespace MEngine {

	MScene::MScene()
	{
		this->scene = new Scene();

		this->ContentManager = gcnew MContentManager(&(*this->scene->ContentManager));
	}

	MScene::~MScene()
	{
		delete this->scene;
		this->scene = NULL;
	}

}