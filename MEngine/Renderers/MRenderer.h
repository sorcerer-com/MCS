// MRenderer.h
#pragma once

#include "Engine\Renderers\Renderer.h"
#pragma managed

#include "..\Utils\MHeader.h"
#include "..\Utils\Types\MPoint.h"


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
			Vector3 dir, inter;
			return this->renderer->GetIntesectionInfo((float)x, (float)y, dir, inter);
		}

		MPoint GetDirection(double x, double y)
		{
			Vector3 dir, inter;
			this->renderer->GetIntesectionInfo((float)x, (float)y, dir, inter);
			return MPoint(dir);
		}

		MPoint GetIntesectionPoint(double x, double y)
		{
			Vector3 dir, inter;
			this->renderer->GetIntesectionInfo((float)x, (float)y, dir, inter);
			return MPoint(inter);
		}

	};

};