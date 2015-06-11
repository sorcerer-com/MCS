// Renderer.cpp

#include "stdafx.h"
#include "Renderer.h"

#include "..\Utils\Thread.h"
#include "..\Utils\Config.h"


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
        this->thread->join();
    }


    /* V I E W P O R T   R E N D E R E R */
    ViewPortRenderer::ViewPortRenderer(Engine* owner, RendererType type) :
        Renderer(owner, type)
    {
        this->Resized = false;
    }

    void ViewPortRenderer::ReSize(int width, int height)
    {
        this->Width = width;
        this->Height = height;
        this->Resized = true;
    }


    /* P R O D U C T I O N   R E N D E R E R */
    ProductionRenderer::ProductionRenderer(Engine* owner, RendererType type) :
        Renderer(owner, type)
    {
    }

    bool ProductionRenderer::Init(int width, int height)
    {
        this->Width = width;
        this->Height = height;
        return true;
    }
}