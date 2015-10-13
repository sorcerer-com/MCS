// RenderElement.h
#pragma once

#include "SceneElement.h"
#include "..\Utils\Header.h"


namespace MyEngine {

    enum RenderElementType
    {
        ESlicer
    };
	
	class RenderElement : public SceneElement
	{
	public:
        RenderElementType RType;            //* default[RenderElementType::ESlicer] group["Render Element"]

	public:
        RenderElement(SceneManager* owner, const string& name, uint contentID);
        RenderElement(SceneManager* owner, istream& file);

		virtual void WriteToFile(ostream& file) const override;
		virtual SceneElement* Clone() const override;

	private:
        void init();
	};

}

