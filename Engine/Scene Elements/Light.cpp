// Light.cpp

#include "stdafx.h"
#include "Light.h"

#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"


namespace MyEngine {

	Light::Light(SceneManager* owner, const string& name, uint contentID, LightType lightType) :
		SceneElement(owner, ELight, name, contentID)
	{
		this->init();
		this->LType = lightType;
	}

	Light::Light(SceneManager* owner, istream& file) :
		SceneElement(owner, file)
	{
		this->init();
		if (this->Version >= 1)
        {
#pragma region Light Read
            Read(file, this->LType);
            Read(file, this->Radius);
            Read(file, this->Color);
            Read(file, this->SpotExponent);
            Read(file, this->SpotCutoff);
            Read(file, this->Intensity);
#pragma endregion
		}
	}

	void Light::init()
    {
#pragma region Light Init
        this->LType = LightType::ESun;
        this->Radius = 128.0f;
        this->Color = Color4(0.8f, 0.8f, 0.8f, 1.0f);
        this->SpotExponent = 0.5f;
        this->SpotCutoff = 180.0f;
        this->Intensity = 128.0f;
#pragma endregion
	}


    void* Light::get(const string& name)
    {
#pragma region Light Get
        if (name == "LType")
            return &this->LType;
        else if (name == "Radius")
            return &this->Radius;
        else if (name == "Color")
            return &this->Color;
        else if (name == "SpotExponent")
            return &this->SpotExponent;
        else if (name == "SpotCutoff")
            return &this->SpotCutoff;
        else if (name == "Intensity")
            return &this->Intensity;
#pragma endregion
        return SceneElement::get(name);
    }

	void Light::WriteToFile(ostream& file) const
	{
		SceneElement::WriteToFile(file);

#pragma region Light Write
		Write(file, this->LType);
		Write(file, this->Radius);
		Write(file, this->Color);
		Write(file, this->SpotExponent);
		Write(file, this->SpotCutoff);
		Write(file, this->Intensity);
#pragma endregion
		file.flush();
	}

	SceneElement* Light::Clone() const
	{
		Light* elem = new Light(*this);
		elem->ID = INVALID_ID;
		return elem;
	}

}
