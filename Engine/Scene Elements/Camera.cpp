// Camera.cpp

#include "stdafx.h"
#include "Camera.h"

#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"


namespace MyEngine {

	Camera::Camera(SceneManager* owner, const string& name, uint contentID) :
		SceneElement(owner, ECamera, name, contentID)
	{
		this->init();
	}

	Camera::Camera(SceneManager* owner, istream& file) :
		SceneElement(owner, file)
	{
		this->init();
		if (this->Version >= 1)
		{
			Read(file, this->FOV);
			Read(file, this->FocalPlaneDist);
			Read(file, this->FNumber);
		}
	}

	void Camera::init()
	{
		this->FOV = 72.0f;
		this->FocalPlaneDist = 0.0f;
		this->FNumber = 2.0f;
	}


	void Camera::Move(const Vector3& v)
	{
		Vector3 vv = this->Rotation * v;
		this->Position += vv;
	}

	Vector3 Camera::GetDirection()
	{
		Vector3 dir(0, 0, 1);
		return this->Rotation * dir;
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