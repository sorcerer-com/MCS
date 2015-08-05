// Renderer.h
#pragma once

#include "..\Utils\Header.h"
#include "..\Utils\Types\Buffer.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

	class Engine;
	struct Thread;
    struct Vector3;
    struct Profiler;
    struct Region;

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
        
        BufferMapType Buffers;
        bool IsStarted;

    private:
        shared_ptr<Profiler> profiler;

    public:
        ProductionRenderer(Engine* owner, RendererType type);
        virtual ~ProductionRenderer() = 0;

        double GetRenderTime(); // in seconds
        virtual vector<string> GetBufferNames() = 0;
        virtual vector<Region> GetActiveRegions() = 0;
        virtual double GetProgress() = 0;
        virtual bool Init(uint width, uint height);
        virtual void Start();
        virtual void Stop();
    };

}