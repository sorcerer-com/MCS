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
		property double FieldOfView
		{
			double get() { return camera->FOV; }
			void set(double value) { camera->FOV = (float)value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Camera")]
		property double FocalPlaneDist
		{
			double get() { return camera->FocalPlaneDist; }
			void set(double value) { camera->FocalPlaneDist = (float)value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Camera")]
		property double FNumber
		{
			double get() { return camera->FNumber; }
			void set(double value) { camera->FNumber = (float)value; OnChanged(); }
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

        void Rotate(double x, double y, double z)
        {
            this->camera->Rotate(Quaternion(Vector3(x, y, z)));
        }

		MPoint GetDirection()
		{
			return MPoint(this->camera->GetDirection());
		}

	};

}