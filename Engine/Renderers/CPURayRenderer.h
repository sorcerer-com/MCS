// CPURayRenderer.h
#pragma once

#include "Renderer.h"
#include "..\Utils\Header.h"


namespace MyEngine {
    	
    class CPURayRenderer : public ProductionRenderer
    {
        // TODO: 
        // images(diffuse, specular, direct light, indirect light, total light, reflection, refraction, depth, final) - map may be?
        // buffer intersection infos
        // flags - max reflection/refraction level, GI, interactive(dynamic scene or not), animation - may be in ProductionRenderer
	private:

	public:
        CPURayRenderer(Engine* owner);
        ~CPURayRenderer();

        virtual bool Init(int width, int height) override;

	private:

	};

}