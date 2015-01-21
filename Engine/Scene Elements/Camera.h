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
		// TODO: exposure?

	public:
		Camera(SceneManager* owner, const string& name, uint contentID);
		Camera(SceneManager* owner, istream& file);

		virtual void WriteToFile(ostream& file) const override;
		virtual SceneElement* Clone() const override;
	};

}

