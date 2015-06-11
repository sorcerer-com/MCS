// IrrRenderer.cpp

#include "stdafx.h"
#include "CPURayRenderer.h"

#pragma warning(push, 3)
#include <Embree\rtcore.h>
#include <Embree\rtcore_geometry.h>
#include <Embree\rtcore_geometry_user.h>
#include <Embree\rtcore_ray.h>
#include <Embree\rtcore_scene.h>
#pragma warning(pop)

#include "..\Engine.h"
#include "..\Utils\Thread.h"
#include "..\Utils\Config.h"


namespace MyEngine {    

    CPURayRenderer::CPURayRenderer(Engine* owner) :
        ProductionRenderer(owner, RendererType::ECPURayRenderer)
    {
    }

    CPURayRenderer::~CPURayRenderer()
    {
        Engine::Log(LogType::ELog, "CPURayRenderer", "DeInit CPU Ray Renderer");
    }

    
    bool CPURayRenderer::Init(int width, int height)
    {
        ProductionRenderer::Init(width, height);
        Engine::Log(LogType::ELog, "CPURayRenderer", "Init CPU Ray Renderer to (" + to_string(width) + ", " + to_string(height) + ")");
        return true;
    }

}