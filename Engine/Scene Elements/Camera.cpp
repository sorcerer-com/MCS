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
#pragma region Camera Read
            Read(file, this->FOV);
            Read(file, this->FocalPlaneDist);
            Read(file, this->FNumber);
#pragma endregion
		}
	}

	void Camera::init()
    {
#pragma region Camera Init
        this->FOV = 72.0f;
        this->FocalPlaneDist = 0.0f;
        this->FNumber = 2.0f;
#pragma endregion
	}


	void Camera::Move(const Vector3& v)
	{
		Vector3 vv = this->Rotation * v;
		this->Position += vv;
    }

    void Camera::Rotate(const Quaternion& q)
    {
        this->Rotation = q * this->Rotation;
        this->Rotation.normalize();
    }

	Vector3 Camera::GetDirection() const
	{
		Vector3 dir(0, 0, 1);
		return this->Rotation * dir;
	}


    void* Camera::get(const string& name)
    {
#pragma region Camera Get
        if (name == "FOV")
            return &this->FOV;
        else if (name == "FocalPlaneDist")
            return &this->FocalPlaneDist;
        else if (name == "FNumber")
            return &this->FNumber;
#pragma endregion
        return SceneElement::get(name);
    }

	void Camera::WriteToFile(ostream& file) const
	{
		SceneElement::WriteToFile(file);

#pragma region Camera Write
		Write(file, this->FOV);
		Write(file, this->FocalPlaneDist);
		Write(file, this->FNumber);
#pragma endregion
		file.flush();
	}

	SceneElement* Camera::Clone() const
	{
		Camera* elem = new Camera(*this);
		elem->ID = INVALID_ID;
		return elem;
	}

}
