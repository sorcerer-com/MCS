// MRenderer.h
#pragma once

#include "Engine\Renderers\Renderer.h"
#pragma managed

#include "..\Utils\MHeader.h"


namespace MyEngine {

	public ref class MRenderer
	{
	private:
		Renderer* renderer;

	public:
		MRenderer(Renderer* renderer)
		{
			this->renderer = renderer;
		}


		void Init(IntPtr handle)
		{
			this->renderer->Init((void*)handle);
		}

		void ReSize(int width, int height)
		{
			this->renderer->ReSize(width, height);
		}

		uint GetSceneElementID(double x, double y)
		{
			return this->renderer->GetSceneElementID((float)x, (float)y);
		}

	};

};