// MViewPortRenderer.h
#pragma once

#include "Engine\Renderers\Renderer.h"
#pragma managed

#include "..\Utils\MHeader.h"
#include "..\Utils\Types\MPoint.h"


namespace MyEngine {

	public enum class ERendererType
	{
		IrrRenderer,
		CPURayRenderer,
		GPURayRenderer
	};

	public ref class MRenderer abstract
	{
	protected:
        Renderer* renderer;

	public:
		property ERendererType Type
		{
			ERendererType get() { return (ERendererType)this->renderer->Type; }
		}

		property int Width
		{
			int get() { return this->renderer->Width; }
		}

		property int Height
		{
			int get() { return this->renderer->Height; }
		}

	public:
        MRenderer(Renderer* renderer)
		{
			this->renderer = renderer;
		}

	};

};