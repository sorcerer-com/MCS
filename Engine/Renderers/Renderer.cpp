// Renderer.cpp

#include "stdafx.h"
#include "Renderer.h"

#include "..\Utils\Thread.h"


namespace MyEngine {

	Renderer::Renderer(Engine* owner, RendererType type)
	{
		if (!owner)
			throw "ArgumentNullException: owner";

		this->Owner = owner;

		this->thread = make_shared<Thread>();

		this->Type = type;
		this->Width = 0;
		this->Height = 0;
		this->Resized = false;
	}

}