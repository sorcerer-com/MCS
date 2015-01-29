// Light.cpp

#include "stdafx.h"
#include "Light.h"

#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"


namespace MyEngine {

	Light::Light(SceneManager* owner, const string& name, uint contentID, LightType lightType) :
		SceneElement(owner, ELight, name, contentID)
	{
		this->LType = lightType;
		this->Radius = 128.0f;
		this->Color = Color4(0.8f, 0.8f, 0.8f, 1.0f);
		this->SpotExponent = 0.5f;
		this->SpotCutoffInner = 0.0f;
		this->SpotCutoffOuter = 180.0f;
		this->Intensity = 128.0f;
	}

	Light::Light(SceneManager* owner, istream& file) :
		SceneElement(owner, file)
	{
		if (this->Version >= 1)
		{
			Read(file, this->LType);
			Read(file, this->Radius);
			Read(file, this->Color);
			Read(file, this->SpotExponent);
			Read(file, this->SpotCutoffInner);
			Read(file, this->SpotCutoffOuter);
			Read(file, this->Intensity);
		}
	}


	void Light::WriteToFile(ostream& file) const
	{
		SceneElement::WriteToFile(file);

		Write(file, this->LType);
		Write(file, this->Radius);
		Write(file, this->Color);
		Write(file, this->SpotExponent);
		Write(file, this->SpotCutoffInner);
		Write(file, this->SpotCutoffOuter);
		Write(file, this->Intensity);
		file.flush();
	}

	SceneElement* Light::Clone() const
	{
		Light* elem = new Light(*this);
		elem->ID = INVALID_ID;
		return elem;
	}

}