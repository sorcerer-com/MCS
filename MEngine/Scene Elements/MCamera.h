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
            Camera* get() { return (Camera*)this->sceneElement; }
		}

    public:
#pragma region Camera Properties
        [MPropertyAttribute(Group = "Camera")]
        property double FOV
        {
            double get() { return this->camera->FOV; }
            void set(double value) { this->camera->FOV = (float)value; OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Camera")]
        property double FocalPlaneDist
        {
            double get() { return this->camera->FocalPlaneDist; }
            void set(double value) { this->camera->FocalPlaneDist = (float)value; OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Camera")]
        property double FNumber
        {
            double get() { return this->camera->FNumber; }
            void set(double value) { this->camera->FNumber = (float)value; OnChanged(); }
        }
#pragma endregion

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

#pragma region Camera Functions
        void Move(MPoint v)
        {
            this->camera->Move(v.ToVector3());
            this->OnChanged();
        }

        void Rotate(MPoint q)
        {
            this->camera->Rotate(Quaternion(q.ToVector3()));
            this->OnChanged();
        }

        MPoint GetDirection()
        {
            return MPoint(this->camera->GetDirection());
        }
#pragma endregion

	};

}
