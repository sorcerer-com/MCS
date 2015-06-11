// MViewPortRenderer.h
#pragma once

#include "Engine\Renderers\Renderer.h"
#pragma managed

#include "MRenderer.h"


namespace MyEngine {
    
    public ref class MProductionRenderer : MRenderer
    {
    protected:
        property ProductionRenderer* Renderer
        {
            ProductionRenderer* get() { return (ProductionRenderer*)this->renderer; }
        }

	public:

	public:
        MProductionRenderer(ProductionRenderer* renderer) :
            MRenderer(renderer)
		{
		}

	};

};