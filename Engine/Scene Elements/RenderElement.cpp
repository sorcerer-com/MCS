// RenderElement.cpp

#include "stdafx.h"
#include "RenderElement.h"

#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"


namespace MyEngine {

    RenderElement::RenderElement(SceneManager* owner, const string& name, uint contentID) :
		SceneElement(owner, ERenderObject, name, contentID)
	{
		this->init();
	}

    RenderElement::RenderElement(SceneManager* owner, istream& file) :
		SceneElement(owner, file)
	{
		this->init();
		if (this->Version >= 1)
        {
#pragma region RenderElement Read
            Read(file, this->RType);
#pragma endregion
		}
	}

    void RenderElement::init()
    {
#pragma region RenderElement Init
        this->RType = RenderElementType::ESlicer;
#pragma endregion
	}

    void RenderElement::WriteToFile(ostream& file) const
	{
		SceneElement::WriteToFile(file);

#pragma region RenderElement Write
		Write(file, this->RType);
#pragma endregion
		file.flush();
	}

    SceneElement* RenderElement::Clone() const
	{
        RenderElement* elem = new RenderElement(*this);
		elem->ID = INVALID_ID;
		return elem;
	}

}
