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
#include "..\Utils\Types\Thread.h"
#include "..\Utils\Config.h"


namespace MyEngine {

    CPURayRenderer::CPURayRenderer(Engine* owner) :
        ProductionRenderer(owner, RendererType::ECPURayRenderer)
    {
        this->RegionSize = 0;
    }

    CPURayRenderer::~CPURayRenderer()
    {
        this->thread->join();
        Engine::Log(LogType::ELog, "CPURayRenderer", "DeInit CPU Ray Renderer");
    }

    
    bool CPURayRenderer::Init(uint width, uint height)
    {
        ProductionRenderer::Init(width, height);
        for (uint i = 0; i < CPURayRenderer::BuffersNamesCount; i++)
            this->Buffers[CPURayRenderer::BuffersNames[i]].Init(width, height);
        this->generateRegions();
        // TODO: init Buckets, do preview and based on bucket render time sort them
        // TODO: log execution time of different phases - may be Profiler?

        Engine::Log(LogType::ELog, "CPURayRenderer", "Init CPU Ray Renderer to (" + to_string(width) + ", " + to_string(height) + ")");
        return true;
    }

    void CPURayRenderer::Start()
    {
        ProductionRenderer::Start();
        this->thread->defThreadPool();

        Engine::Log(LogType::ELog, "CPURayRenderer", "Start Rendering");
    }

    void CPURayRenderer::Stop()
    {
        ProductionRenderer::Stop();
        this->thread->join();

        Engine::Log(LogType::ELog, "CPURayRenderer", "Stop Rendering");
    }


    void CPURayRenderer::generateRegions()
    {
        int sw = (this->Width - 1) / this->RegionSize + 1;
        int sh = (this->Height -1) / this->RegionSize + 1;
        for (int y = 0; y < sh; y++)
        {
            for (int x = 0; x < sw; x++)
            {
                int left = x * this->RegionSize;
                int right = min(this->Width, (x + 1) * this->RegionSize);
                int top = y * this->RegionSize;
                int bottom = min(this->Height, (y + 1) * this->RegionSize);
                if (left == right || top == bottom)
                    continue;

                this->Regions.push_back(Region(left, top, right - left, bottom - top));
            }
        }
    }

}