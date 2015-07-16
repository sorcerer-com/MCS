// MViewPortRenderer.h
#pragma once

#include "Engine\Renderers\Renderer.h"
#include "Engine\Renderers\CPURayRenderer.h"
#pragma managed

#include "MRenderer.h"

using namespace System::Drawing;

namespace MyEngine {
    
    public ref class MProductionRenderer : MRenderer
    {
    protected:
        property ProductionRenderer* Renderer
        {
            ProductionRenderer* get() { return (ProductionRenderer*)this->renderer; }
        }

        Bitmap^ buffer;

	public:
        ref struct MRenderSettings
        {
        public:
            [MPropertyAttribute(SortName = "001", Group = "Main Settings")]
            property uint Width;
            [MPropertyAttribute(SortName = "002", Group = "Main Settings")]
            property uint Height;
            [MPropertyAttribute(SortName = "003", Group = "Main Settings")]
            property uint RegionSize;
            [MPropertyAttribute(SortName = "004", Group = "Samples Settings")]
            property uint MinSamples;
            [MPropertyAttribute(SortName = "005", Group = "Samples Settings")]
            property uint MaxSamples;
            [MPropertyAttribute(SortName = "006", Group = "Samples Settings")]
            property double SamplesThreshold;
            [MPropertyAttribute(SortName = "007", Group = "Samples Settings")]
            property uint MaxLights;
        };

        property bool IsStarted
        {
            bool get() { return this->Renderer->IsStarted; }
        }

        property List<String^>^ BuffersNames
        {
            List<String^>^ get()
            {
                List<String^>^ collection = gcnew List<String^>();

                const auto& bufferNames = this->Renderer->GetBufferNames();
                for (const auto& bufferName : bufferNames)
                    collection->Add(gcnew String(bufferName.c_str()));

                return collection;
            }
        }

        property List<Rectangle>^ ActiveRegions
        {
            List<Rectangle>^ get()
            {
                List<Rectangle>^ collection = gcnew List<Rectangle>();

                const auto& activeRegions = this->Renderer->GetActiveRegions();
                for (const auto& region : activeRegions)
                    collection->Add(Rectangle(region.x, region.y, region.w, region.h));

                return collection;
            }
        }

	public:
        MProductionRenderer(ProductionRenderer* renderer) :
            MRenderer(renderer)
		{
        }


        void Init(MRenderSettings^ settings)
        {
            if (this->Type == ERendererType::CPURayRenderer)
            {
                CPURayRenderer* renderer = (CPURayRenderer*)this->Renderer;
                renderer->RegionSize = settings->RegionSize;
                renderer->MinSamples = settings->MinSamples;
                renderer->MaxSamples = settings->MaxSamples;
                renderer->SamplesThreshold = (float)settings->SamplesThreshold;
                renderer->MaxLights = settings->MaxLights;
            }
            this->Renderer->Init(settings->Width, settings->Height);

            this->buffer = gcnew Bitmap(settings->Width, settings->Height, Imaging::PixelFormat::Format32bppArgb);
        }

        void Start()
        {
            this->Renderer->Start();
        }

        void Stop()
        {
            this->Renderer->Stop();
        }

        Bitmap^ GetBuffer(String^ name)
        {
            if (this->Renderer->Buffers.find(to_string(name)) == this->Renderer->Buffers.end()) // doesn't contin
                return nullptr;

            Imaging::BitmapData^ data =
                this->buffer->LockBits(System::Drawing::Rectangle(0, 0, this->buffer->Width, this->buffer->Height),
                Imaging::ImageLockMode::WriteOnly, Imaging::PixelFormat::Format32bppArgb);
            const auto& buffer = this->Renderer->Buffers[to_string(name)];
            byte* dataByte = (byte*)data->Scan0.ToPointer();
            for (uint i = 0; i < buffer.width * buffer.height; i++)
            {
                // from RGBA to BGRA
                dataByte[i * 4 + 2] = (byte)(std::min(buffer.data[i].r, 1.0f) * 255);
                dataByte[i * 4 + 1] = (byte)(std::min(buffer.data[i].g, 1.0f) * 255);
                dataByte[i * 4 + 0] = (byte)(std::min(buffer.data[i].b, 1.0f) * 255);
                dataByte[i * 4 + 3] = (byte)(std::min(buffer.data[i].a, 1.0f) * 255);
            }
            //scene->Renderer->ImageMutex.lock();
            //memcpy((void*)data->Scan0, this->Renderer->Image.Pixels, scene->Renderer->Image.Width * scene->Renderer->Image.Height * scene->Renderer->Image.Components);
            //scene->Renderer->ImageMutex.unlock();
            this->buffer->UnlockBits(data);
            return this->buffer;
        }

	};

};