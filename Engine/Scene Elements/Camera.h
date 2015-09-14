// Camera.h
#pragma once

#include "SceneElement.h"
#include "..\Utils\Header.h"


namespace MyEngine {
	
	class Camera : public SceneElement
	{
	public:
		float FOV;                          //* default[72.0f] group["Camera"]
		float FocalPlaneDist;               //* default[0.0f] group["Camera"]
		float FNumber;                      //* default[2.0f] group["Camera"]
		// TODO: exposure, near/far plane?

	public:
		Camera(SceneManager* owner, const string& name, uint contentID);
		Camera(SceneManager* owner, istream& file);

		void Move(const Vector3& v);        //* wrap
        void Rotate(const Quaternion& q);   //* wrap
		Vector3 GetDirection() const;       //* wrap

		virtual void WriteToFile(ostream& file) const override;
		virtual SceneElement* Clone() const override;

	private:
        void init();
        virtual void* get(const string& name);
	};

}

