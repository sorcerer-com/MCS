// MViewPortRenderer.h
#pragma once

#include "Engine\Renderers\Renderer.h"
#pragma managed

#include "MRenderer.h"


namespace MyEngine {

    public ref class MViewPortRenderer : MRenderer
	{
    protected:
        property ViewPortRenderer* Renderer
        {
            ViewPortRenderer* get() { return (ViewPortRenderer*)this->renderer; }
        }

	public:
		property bool Resized
		{
            bool get() { return this->Renderer->Resized; }
		}

	public:
        MViewPortRenderer(ViewPortRenderer* renderer) :
            MRenderer(renderer)
		{
		}


		void Init(IntPtr handle)
		{
            this->Renderer->Init((void*)handle);
		}

        void ReSize(uint width, uint height)
		{
            this->Renderer->ReSize(width, height);
		}


		uint GetSceneElementID(double x, double y)
		{
			Vector3 dir, inter;
            return this->Renderer->GetIntesectionInfo((float)x, (float)y, dir, inter);
		}

		MPoint GetDirection(double x, double y)
		{
			Vector3 dir, inter;
            this->Renderer->GetIntesectionInfo((float)x, (float)y, dir, inter);
			return MPoint(dir);
		}

		MPoint GetIntesectionPoint(double x, double y)
		{
			Vector3 dir, inter;
            this->Renderer->GetIntesectionInfo((float)x, (float)y, dir, inter);
			return MPoint(inter);
		}

	};

};