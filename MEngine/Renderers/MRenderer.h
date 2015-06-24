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

        property uint Width
        {
            uint get() { return this->renderer->Width; }
        }

        property uint Height
        {
            uint get() { return this->renderer->Height; }
        }


        static property List<ERendererType>^ ViewPortRendererTypes
        {
            List<ERendererType>^ get()
            {
                List<ERendererType>^ collection = gcnew List<ERendererType>();
                collection->Add(ERendererType::IrrRenderer);
                return collection;
            }
        }

        static property List<ERendererType>^ ProductionRendererTypes
        {
            List<ERendererType>^ get()
            {
                List<ERendererType>^ collection = gcnew List<ERendererType>();
                collection->Add(ERendererType::CPURayRenderer);
                collection->Add(ERendererType::GPURayRenderer);
                return collection;
            }
        }

    public:
        MRenderer(Renderer* renderer)
        {
            this->renderer = renderer;
        }

	};

};