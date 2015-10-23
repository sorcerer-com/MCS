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
            [MPropertyAttribute(SortName = "01", Group = "01. Main Settings")]
            property uint Width;
            [MPropertyAttribute(SortName = "02", Group = "01. Main Settings")]
            property uint Height;
            [MPropertyAttribute(SortName = "03", Group = "01. Main Settings")]
            property uint RegionSize;
            [MPropertyAttribute(SortName = "04", Group = "01. Main Settings")]
            property bool VolumetricFog;
            [MPropertyAttribute(SortName = "01", Group = "02. Samples Settings")]
            property uint MinSamples;
            [MPropertyAttribute(SortName = "02", Group = "02. Samples Settings")]
            property uint MaxSamples;
            [MPropertyAttribute(SortName = "03", Group = "02. Samples Settings")]
            property double SampleThreshold;
            [MPropertyAttribute(SortName = "01", Group = "03. Limits")]
            property uint MaxLights;
            [MPropertyAttribute(SortName = "02", Group = "03. Limits")]
            property uint MaxDepth;
            [MPropertyAttribute(SortName = "01", Group = "04. Global Illumination")]
            property bool GI;
            [MPropertyAttribute(SortName = "02", Group = "04. Global Illumination", Name = "Samples")]
            property uint GISamples;
            [MPropertyAttribute(SortName = "03", Group = "04. Global Illumination")]
            property bool IrradianceMap;
            [MPropertyAttribute(SortName = "04", Group = "04. Global Illumination", Name = "Samples")]
            property uint IrradianceMapSamples;
            [MPropertyAttribute(SortName = "05", Group = "04. Global Illumination", Name = "DistanceThreshold")]
            property double IrradianceMapDistanceThreshold;
            [MPropertyAttribute(SortName = "06", Group = "04. Global Illumination", Name = "NormalThreshold")]
            property double IrradianceMapNormalThreshold;
            [MPropertyAttribute(SortName = "07", Group = "04. Global Illumination", Name = "ColorThreshold")]
            property double IrradianceMapColorThreshold;
            [MPropertyAttribute(SortName = "08", Group = "04. Global Illumination")]
            property bool LightCache;
            [MPropertyAttribute(SortName = "09", Group = "04. Global Illumination", Name = "SampleSize")]
            property double LightCacheSampleSize;
            [MPropertyAttribute(SortName = "01", Group = "05. Animation")]
            property bool Animation;
            [MPropertyAttribute(SortName = "02", Group = "05. Animation", Name = "FPS")]
            property int AnimationFPS;
            [MPropertyAttribute(SortName = "03", Group = "05. Animation", Name = "ResetCaches")]
            property bool AnimationResetCaches;
        };

        property bool IsStarted
        {
            bool get() { return this->Renderer->IsStarted; }
        }

        property double Exposure;

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

                const auto& regions = this->Renderer->GetActiveRegions();
                for (const auto& region : regions)
                    collection->Add(Rectangle(region.x, region.y, region.w, region.h));

                return collection;
            }
        }

        property TimeSpan RenderTime
        {
            TimeSpan get() { return TimeSpan::FromSeconds(this->Renderer->GetRenderTime()); }
        }

        property double Progress
        {
            double get() { return this->Renderer->GetProgress(); }
        }

	public:
        MProductionRenderer(ProductionRenderer* renderer) :
            MRenderer(renderer)
		{
            this->Exposure = 1.0;
        }


        void Init(MRenderSettings^ settings)
        {
            if (this->Type == ERendererType::CPURayRenderer)
            {
                CPURayRenderer* rayRenderer = (CPURayRenderer*)this->Renderer;
                rayRenderer->RegionSize = settings->RegionSize;
                rayRenderer->VolumetricFog = settings->VolumetricFog;
                rayRenderer->MinSamples = settings->MinSamples;
                rayRenderer->MaxSamples = settings->MaxSamples;
                rayRenderer->SampleThreshold = (float)settings->SampleThreshold;
                rayRenderer->MaxLights = settings->MaxLights;
                rayRenderer->MaxDepth = settings->MaxDepth;
                rayRenderer->GI = settings->GI;
                rayRenderer->GISamples = settings->GISamples;
                rayRenderer->IrradianceMap = settings->IrradianceMap;
                rayRenderer->IrradianceMapSamples = settings->IrradianceMapSamples;
                rayRenderer->IrradianceMapDistanceThreshold = (float)settings->IrradianceMapDistanceThreshold;
                rayRenderer->IrradianceMapNormalThreshold = (float)settings->IrradianceMapNormalThreshold;
                rayRenderer->IrradianceMapColorThreshold = (float)settings->IrradianceMapColorThreshold;
                rayRenderer->LightCache = settings->LightCache;
                rayRenderer->LightCacheSampleSize = (float)settings->LightCacheSampleSize;
                rayRenderer->Animation = settings->Animation;
                rayRenderer->AnimationFPS = settings->AnimationFPS;
                rayRenderer->AnimationResetCaches = settings->AnimationResetCaches;
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
                dataByte[i * 4 + 2] = (byte)(min(buffer.data[i].r * (float)this->Exposure, 1.0f) * 255);
                dataByte[i * 4 + 1] = (byte)(min(buffer.data[i].g * (float)this->Exposure, 1.0f) * 255);
                dataByte[i * 4 + 0] = (byte)(min(buffer.data[i].b * (float)this->Exposure, 1.0f) * 255);
                dataByte[i * 4 + 3] = (byte)(min(buffer.data[i].a * (float)this->Exposure, 1.0f) * 255);
            }
            this->buffer->UnlockBits(data);
            return this->buffer;
        }

	};

};