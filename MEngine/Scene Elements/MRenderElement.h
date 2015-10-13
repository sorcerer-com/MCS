// MRenderElement.h
#pragma once

#include "Engine\Scene Elements\RenderElement.h"
#pragma managed

#include "MSceneElement.h"
#include "..\Utils\MHeader.h"


namespace MyEngine {

    public enum class ERenderElementType
    {
        Slicer
    };

    public ref class MRenderElement : MSceneElement
	{
	private:
        property RenderElement* renderElement
		{
            RenderElement* get() { return (RenderElement*)this->sceneElement; }
		}

    public:
#pragma region RenderElement Properties
        [MPropertyAttribute(Group = "Render Element")]
        property ERenderElementType RType
        {
            ERenderElementType get() { return (ERenderElementType)this->renderElement->RType; }
            void set(ERenderElementType value) { this->renderElement->RType = (RenderElementType)value; OnChanged(); }
        }
#pragma endregion

	public:
        MRenderElement(SceneManager* owner, uint id) :
			MSceneElement(owner, id)
		{
        }

#pragma region Camera Functions
#pragma endregion

	};

}
