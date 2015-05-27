// Light.h
#pragma once

#include "SceneElement.h"
#include "..\Utils\Header.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

	enum LightType
	{
		ESun,
		EStaticLight
	};

	class Light : public SceneElement
	{
	public:
		LightType LType;
		float Radius;
		Color4 Color;
		float SpotExponent;
		float SpotCutoffInner;
		float SpotCutoffOuter;
		float Intensity; // 0 - light is off, else on

	public:
		Light(SceneManager* owner, const string& name, uint contentID, LightType lightType);
		Light(SceneManager* owner, istream& file);

		virtual void WriteToFile(ostream& file) const override;
		virtual SceneElement* Clone() const override;

	private:
		void init();
	};

}

