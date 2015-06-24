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
                for (uint i = 0; i < bufferNames.size(); i++)
                    collection->Add(gcnew String(bufferNames[i].c_str()));

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
            }
            this->Renderer->Init(settings->Width, settings->Height);

            this->buffer = gcnew Bitmap(settings->Width, settings->Height, Imaging::PixelFormat::Format32bppRgb);
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
                Imaging::ImageLockMode::WriteOnly, Imaging::PixelFormat::Format32bppRgb);
            const auto& buffer = this->Renderer->Buffers[to_string(name)];
            byte* dataByte = (byte*)data->Scan0.ToPointer();
            for (uint i = 0; i < buffer.width * buffer.height; i++)
            {
                // from RGBA to BGRA
                dataByte[i * 4 + 2] = (byte)(buffer.data[i].r * 255);
                dataByte[i * 4 + 1] = (byte)(buffer.data[i].g * 255);
                dataByte[i * 4 + 0] = (byte)(buffer.data[i].b * 255);
                dataByte[i * 4 + 3] = (byte)(buffer.data[i].a * 255);
            }
            //scene->Renderer->ImageMutex.lock();
            //memcpy((void*)data->Scan0, this->Renderer->Image.Pixels, scene->Renderer->Image.Width * scene->Renderer->Image.Height * scene->Renderer->Image.Components);
            //scene->Renderer->ImageMutex.unlock();
            this->buffer->UnlockBits(data);
            this->buffer->RotateFlip(RotateFlipType::RotateNoneFlipY);
            return this->buffer;
        }

	};

};