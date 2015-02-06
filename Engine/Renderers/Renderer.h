// Renderer.h
#pragma once

#include "..\Utils\Header.h"


namespace MyEngine {

	class Engine;
	struct Thread;

	enum RendererType
	{
		EIrrRenderer,
		ECPURayRenderer,
		EGPURayRenderer
	};

	class Renderer
	{
	protected:
		shared_ptr<Thread> thread;

	public:
		Engine* Owner;

		RendererType Type;
		int Width;
		int Height;
		bool Resized;

	public:
		Renderer(Engine* owner, RendererType type);
		
		virtual bool Init(void* params) = 0;
		virtual void ReSize(int width, int height) = 0;
		virtual uint GetSceneElementID(float x, float y) = 0;
	};

	// TODO: may be subclass for "production" renderers
}