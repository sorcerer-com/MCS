// Renderer.cpp

#include "stdafx.h"
#include "Renderer.h"

#include "..\Utils\Config.h"
#include "..\Utils\Types\Thread.h"


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
    const uint ProductionRenderer::BuffersNamesCount = 9;
    const vector<string> ProductionRenderer::BuffersNames = { "Diffuse", "Specular", "Reflection", "Refraction", "DirectLight", "IndirectLight", "TotalLight", "Depth", "Final" };

    ProductionRenderer::ProductionRenderer(Engine* owner, RendererType type) :
        Renderer(owner, type)
    {
        this->IsStarted = false;
    }

    ProductionRenderer::~ProductionRenderer()
    {
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
    }

    void ProductionRenderer::Stop()
    {
        this->IsStarted = false;
    }

    bool ProductionRenderer::ContainsBuffer(string name)
    {
        return this->Buffers.find(name) != this->Buffers.end();
    }

}