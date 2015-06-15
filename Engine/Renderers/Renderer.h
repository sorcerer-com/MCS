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
		uint Width;
		uint Height;

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
        virtual ~ViewPortRenderer() = 0;

        virtual bool Init(void* params) = 0;
        virtual void ReSize(uint width, uint uheight);
        virtual uint GetIntesectionInfo(float x, float y, Vector3& dir, Vector3& inter) = 0;
    };


    class ProductionRenderer : public Renderer
    {
    public:
        using BufferMapType = map<string, Buffer<Color4>>; // name / buffer

        static const uint BuffersNamesCount;
        static const vector<string> BuffersNames;

        BufferMapType Buffers;
        bool IsStarted;

    public:
        ProductionRenderer(Engine* owner, RendererType type);
        virtual ~ProductionRenderer() = 0;
        // TODO: add timer for rendering

        virtual bool Init(uint width, uint height);
        virtual void Start();
        virtual void Stop();

        bool ContainsBuffer(string name);
    };

}