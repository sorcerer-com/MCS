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
		LightType LType;                    //* default[LightType::ESun] group["Light"]
		float Radius;                       //* default[128.0f] group["Light"]
		Color4 Color;                       //* default[Color4(0.8f, 0.8f, 0.8f, 1.0f)] group["Light"]
		float SpotExponent;                 //* default[0.5f] group["Light"]
		float SpotCutoff;                   //* default[180.0f] group["Light"]
		float Intensity;                    //* default[128.0f] group["Light"]    // 0 - light is off, else on

	public:
		Light(SceneManager* owner, const string& name, uint contentID, LightType lightType);
		Light(SceneManager* owner, istream& file);

		virtual void WriteToFile(ostream& file) const override;
		virtual SceneElement* Clone() const override;

    protected:
        void init();
        virtual void* get(const string& name);
	};

}

