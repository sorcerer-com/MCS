// MLight.h
#pragma once

#include "Engine\Scene Elements\Light.h"
#pragma managed

#include "MSceneElement.h"
#include "..\Utils\MHeader.h"
#include "..\Utils\Types\MColor.h"


namespace MyEngine {

	public enum class ELightType
	{
		Sun,
		Static,
	};

	public ref class MLight : MSceneElement
	{
	private:
		property Light* light
		{
			Light* get() { return (Light*)this->element; }
		}

	public:
		[MPropertyAttribute(Group = "Light")]
		property ELightType LType
		{
			ELightType get() { return (ELightType)light->LType; }
			void set(ELightType value) { light->LType = (LightType)value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Light")]
		property double Radius
		{
			double get() { return light->Radius; }
			void set(double value) { light->Radius = (float)value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Light")]
		property MColor Color
		{
			MColor get() { return MColor(light->Color); }
			void set(MColor value) { light->Color = value.ToColor4(); OnChanged(); }
		}

		[MPropertyAttribute(Group = "Light")]
		property double SpotExponent
		{
			double get() { return light->SpotExponent; }
			void set(double value) { light->SpotExponent = (float)value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Light")]
		property double SpotCutoff
		{
			double get() { return light->SpotCutoff; }
			void set(double value) { light->SpotCutoff = (float)value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Light")]
		property double Intensity
		{
			double get() { return light->Intensity; }
			void set(double value) { light->Intensity = (float)value; OnChanged(); }
		}

	public:
		MLight(SceneManager* owner, uint id) :
			MSceneElement(owner, id)
		{
		}
	};

}