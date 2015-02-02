// MCamera.h
#pragma once

#include "Engine\Scene Elements\Camera.h"
#pragma managed

#include "MSceneElement.h"
#include "..\Utils\MHeader.h"


namespace MyEngine {

	public ref class MCamera : MSceneElement
	{
	private:
		property Camera* camera
		{
			Camera* get() { return (Camera*)this->element; }
		}

	public:
		[MPropertyAttribute(Group = "Camera")]
		property float FieldOfView
		{
			float get() { return camera->FOV; }
			void set(float value) { camera->FOV = value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Camera")]
		property float FocalPlaneDist
		{
			float get() { return camera->FocalPlaneDist; }
			void set(float value) { camera->FocalPlaneDist = value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Camera")]
		property float FNumber
		{
			float get() { return camera->FNumber; }
			void set(float value) { camera->FNumber = value; OnChanged(); }
		}

	public:
		MCamera(SceneManager* owner, uint id) : 
			MSceneElement(owner, id)
		{
		}

		void Move(double x, double y, double z)
		{
			this->camera->Move(Vector3(x, y, z));
		}
	};

}