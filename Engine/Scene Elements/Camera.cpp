// Camera.cpp

#include "stdafx.h"
#include "Camera.h"

#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"


namespace MyEngine {

	Camera::Camera(SceneManager* owner, const string& name, uint contentID) :
		SceneElement(owner, ECamera, name, contentID)
	{
		this->FOV = 72.0f;
		this->FocalPlaneDist = 0.0f;
		this->FNumber = 2.0f;
	}

	Camera::Camera(SceneManager* owner, istream& file) :
		SceneElement(owner, file)
	{
		if (this->Version >= 1)
		{
			Read(file, this->FOV);
			Read(file, this->FocalPlaneDist);
			Read(file, this->FNumber);
		}
	}


	void Camera::WriteToFile(ostream& file) const
	{
		SceneElement::WriteToFile(file);

		Write(file, this->FOV);
		Write(file, this->FocalPlaneDist);
		Write(file, this->FNumber);
		file.flush();
	}

	SceneElement* Camera::Clone() const
	{
		Camera* elem = new Camera(*this);
		elem->ID = INVALID_ID;
		return elem;
	}

}