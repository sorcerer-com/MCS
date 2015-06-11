// Renderer.h
#pragma once

#include "..\Utils\Header.h"
#include "..\Utils\Types\Buffer.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

	class Engine;
	struct Thread;
	struct Vector3;

	enum RendererType
	{
		EIrrRenderer,
		ECPURayRenderer,
		EGPURayRenderer
	};

	class Renderer
	{
	protected:
		shared_ptr<Thread> thread;

	public:
		Engine* Owner;

		RendererType Type;
		int Width;
		int Height;

	public:
		Renderer(Engine* owner, RendererType type);
        virtual ~Renderer() = 0;
	};

    class ViewPortRenderer : public Renderer
    {
    public:
        bool Resized;

    public:
        ViewPortRenderer(Engine* owner, RendererType type);

        virtual bool Init(void* params) = 0;
        virtual void ReSize(int width, int height);
        virtual uint GetIntesectionInfo(float x, float y, Vector3& dir, Vector3& inter) = 0;
    };

    class ProductionRenderer : public Renderer
    {
    public:
        using BufferMapType = map<string, Buffer<Color4>>; // name / buffer

        BufferMapType Buffers;

    public:
        ProductionRenderer(Engine* owner, RendererType type);
        // TODO: add abstract function to make class abstract

        virtual bool Init(int width, int height);
    };

}