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
            Light* get() { return (Light*)this->sceneElement; }
		}

    public:
#pragma region Light Properties
        [MPropertyAttribute(Group = "Light")]
        property ELightType LType
        {
            ELightType get() { return (ELightType)this->light->LType; }
            void set(ELightType value) { this->light->LType = (LightType)value; OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Light")]
        property double Radius
        {
            double get() { return this->light->Radius; }
            void set(double value) { this->light->Radius = (float)value; OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Light")]
        property MColor Color
        {
            MColor get() { return MColor(this->light->Color); }
            void set(MColor value) { this->light->Color = value.ToColor4(); OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Light")]
        property double SpotExponent
        {
            double get() { return this->light->SpotExponent; }
            void set(double value) { this->light->SpotExponent = (float)value; OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Light")]
        property double SpotCutoff
        {
            double get() { return this->light->SpotCutoff; }
            void set(double value) { this->light->SpotCutoff = (float)value; OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Light")]
        property double Intensity
        {
            double get() { return this->light->Intensity; }
            void set(double value) { this->light->Intensity = (float)value; OnChanged(); }
        }
#pragma endregion

	public:
		MLight(SceneManager* owner, uint id) :
			MSceneElement(owner, id)
		{
        }


#pragma region Light Functions
#pragma endregion

	};

}
