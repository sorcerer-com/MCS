// CPURayRenderer.h
#pragma once

#include "Renderer.h"
#include "..\Utils\Header.h"
#include "..\Utils\RayUtils.h"


namespace MyEngine {
    	
    class CPURayRenderer : public ProductionRenderer
    {
        // TODO: 
        // images(diffuse, specular, direct light, indirect light, total light, reflection, refraction, depth, final) - map may be?
        // buffer intersection infos
        // flags - max reflection/refraction level, GI, interactive(dynamic scene or not), animation - may be in ProductionRenderer
	public:
        uint RegionSize;
        vector<Region> Regions;

	public:
        CPURayRenderer(Engine* owner);
        ~CPURayRenderer();

        virtual bool Init(uint width, uint height) override;
        virtual void Start() override;
        virtual void Stop() override;

	private:
        void generateRegions();

	};

}