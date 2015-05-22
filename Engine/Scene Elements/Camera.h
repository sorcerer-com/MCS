// Camera.h
#pragma once

#include "SceneElement.h"
#include "..\Utils\Header.h"


namespace MyEngine {
	
	class Camera : public SceneElement
	{
	public:
		float FOV;
		float FocalPlaneDist;
		float FNumber;
		// TODO: exposure, near plane?

	public:
		Camera(SceneManager* owner, const string& name, uint contentID);
		Camera(SceneManager* owner, istream& file);

		void Move(const Vector3& v);
        void Rotate(const Quaternion& q);
		Vector3 GetDirection();

		virtual void WriteToFile(ostream& file) const override;
		virtual SceneElement* Clone() const override;

	private:
		void init();
	};

}

