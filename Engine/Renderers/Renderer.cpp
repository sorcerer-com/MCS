// Renderer.cpp

#include "stdafx.h"
#include "Renderer.h"

#include "..\Utils\Config.h"
#include "..\Utils\Types\Thread.h"
#include "..\Utils\Types\Profiler.h"


namespace MyEngine {

    /* R E N D E R E R */
	Renderer::Renderer(Engine* owner, RendererType type)
	{
		if (!owner)
			throw "ArgumentNullException: owner";

		this->Owner = owner;

		this->thread = make_shared<Thread>();

		this->Type = type;
		this->Width = 0;
		this->Height = 0;
    }

    Renderer::~Renderer()
    {
    }


    /* V I E W P O R T   R E N D E R E R */
    ViewPortRenderer::ViewPortRenderer(Engine* owner, RendererType type) :
        Renderer(owner, type)
    {
        this->Resized = false;
    }

    ViewPortRenderer::~ViewPortRenderer()
    {
    }

    void ViewPortRenderer::ReSize(uint width, uint height)
    {
        this->Width = width;
        this->Height = height;
        this->Resized = true;
    }


    /* P R O D U C T I O N   R E N D E R E R */
    ProductionRenderer::ProductionRenderer(Engine* owner, RendererType type) :
        Renderer(owner, type)
    {
        this->IsStarted = false;
        this->profiler = make_shared<Profiler>();
    }

    ProductionRenderer::~ProductionRenderer()
    {
        if (this->IsStarted)
            this->Stop();
    }

    bool ProductionRenderer::Init(uint width, uint height)
    {
        this->Width = width;
        this->Height = height;
        this->Buffers.clear();

        return true;
    }

    void ProductionRenderer::Start()
    {
        this->IsStarted = true;
        this->profiler->start();
    }

    void ProductionRenderer::Stop()
    {
        this->IsStarted = false;
        chrono::system_clock::duration delta = this->profiler->stop();
        Engine::Log(LogType::ELog, "ProductionRenderer", "Render time " + duration_to_string(delta));
    }

}