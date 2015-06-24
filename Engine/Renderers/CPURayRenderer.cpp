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
#include "..\Utils\Config.h"
#include "..\Utils\Types\Random.h"
#include "..\Utils\Types\Thread.h"
#include "..\Utils\Types\Profiler.h"
#include "..\Managers\SceneManager.h"
#include "..\Scene Elements\Camera.h"


namespace MyEngine {

    pair<unsigned, Random> Random::rg_table[Random::RGENS];
    
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


    vector<string> CPURayRenderer::GetBufferNames()
    {
        return { "Diffuse", "Specular", "Reflection", "Refraction", "DirectLight", "IndirectLight", "TotalLight", "Depth", "Final" };
    }

    bool CPURayRenderer::Init(uint width, uint height)
    {
        ProfileLog;
        ProductionRenderer::Init(width, height);

        const auto& buffersNames = this->GetBufferNames();
        for (uint i = 0; i < buffersNames.size(); i++)
            this->Buffers[buffersNames[i]].init(width, height);

        Random::initRandom((int)Now);
        this->generateRegions();

        rtcSetErrorFunction((RTCErrorFunc)&onErrorRTC);
        
        Engine::Log(LogType::ELog, "CPURayRenderer", "Init CPU Ray Renderer to (" + to_string(width) + ", " + to_string(height) + ")");
        return true;
    }

    void CPURayRenderer::Start()
    {
        ProductionRenderer::Start();
        this->thread->defThreadPool();

        // TODO: do preview and based on bucket render time sort them
        rtcInit();
        this->beginFrame();

        Engine::Log(LogType::ELog, "CPURayRenderer", "Start Rendering");
    }

    void CPURayRenderer::Stop()
    {
        rtcExit();

        this->thread->join();
        ProductionRenderer::Stop();

        Engine::Log(LogType::ELog, "CPURayRenderer", "Stop Rendering");
    }


    void CPURayRenderer::generateRegions()
    {
        this->Regions.clear();
        int sw = (this->Width - 1) / this->RegionSize + 1;
        int sh = (this->Height -1) / this->RegionSize + 1;
        for (int y = 0; y < sh; y++)
        {
            for (int x = 0; x < sw; x++)
            {
                int left = x * this->RegionSize;
                int right = std::min(this->Width, (x + 1) * this->RegionSize);
                int top = y * this->RegionSize;
                int bottom = std::min(this->Height, (y + 1) * this->RegionSize);
                if (left == right || top == bottom)
                    continue;

                this->Regions.push_back(Region(left, top, right - left, bottom - top));
            }
        }
    }

    void CPURayRenderer::beginFrame()
    {
        Camera* camera = this->Owner->SceneManager->ActiveCamera;

        upLeft = Vector3(-((float)this->Width / this->Height), 1.0f, 0.0f);
        float halfAngle = ((camera ? camera->FOV : 72.0f) / 2.0f) * PI / 180.0f;
        upLeft.normalize();
        upLeft *= (float)tan(halfAngle);
        upLeft.z = -1.0f;

        Vector3 upRight(upLeft);
        upRight.x = -upRight.x;
        Vector3 downLeft(upLeft);
        downLeft.y = -downLeft.y;

        up = Vector3(0.0f, 1.0f, 0.0f);
        right = Vector3(1.0f, 0.0f, 0.0f);
        front = Vector3(0.0f, 0.0f, -1.0f);

        if (camera)
        {
            upLeft = camera->Rotation * upLeft;
            upRight = camera->Rotation * upRight;
            downLeft = camera->Rotation * downLeft;

            up = camera->Rotation * up;
            right = camera->Rotation * right;
            front = camera->Rotation * front;
        }

        dx = (upRight - upLeft) * (1.0f / this->Width);
        dy = (downLeft - upLeft) * (1.0f / this->Height);
    }


    void CPURayRenderer::onErrorRTC(const RTCError, const char* str)
    {
        Engine::Log(LogType::EError, "CPURayRenderer", str);
    }

}