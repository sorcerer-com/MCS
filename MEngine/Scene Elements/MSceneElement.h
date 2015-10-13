// MSceneElement.h
#pragma once

#include "Engine\Scene Elements\SceneElement.h"
#include "Engine\Managers\SceneManager.h"
#pragma managed

#include "..\Utils\MHeader.h"
#include "..\Utils\Types\MPoint.h"

#include "..\Managers\MContentManager.h"
#include "..\Content Elements\MContentElement.h"


namespace MyEngine {
	
	public enum class ESceneElementType
	{
		Camera,
        Light,
        SkyBox,
		RenderObject,
        StaticObject,
        DynamicObject,
	};

	public ref class MSceneElement
	{
	protected:
		SceneManager* owner;
		uint id;

        property SceneElement* sceneElement
		{
			SceneElement* get() { return this->owner->GetElement(this->id).get(); }
		}

    public:
#pragma region SceneElement Properties
        property uint Version
        {
            uint get() { return this->sceneElement->Version; }
        }
        
        [MPropertyAttribute(Group = "Base")]
        property ESceneElementType Type
        {
            ESceneElementType get() { return (ESceneElementType)this->sceneElement->Type; }
        }
        
        [MPropertyAttribute(Group = "Base")]
        property uint ID
        {
            uint get() { return this->sceneElement->ID; }
        }
        
        [MPropertyAttribute(Group = "Base")]
        property String^ Name
        {
            String^ get() { return gcnew String(this->sceneElement->Name.c_str()); }
        }
        
        [MPropertyAttribute(Group = "Base")]
        property String^ Layer
        {
            String^ get() { return gcnew String(this->sceneElement->Layer.c_str()); }
        }
        
        [MPropertyAttribute(Group = "Content", Choosable = true)]
        property MContentElement^ Content
        {
            MContentElement^ get() { return MContentManager::GetMContentElement(this->sceneElement->GetContent()); }
            void set(MContentElement^ value) { this->sceneElement->ContentID = (value != nullptr ? value->ID : 0); OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Content", Choosable = true)]
        property MContentElement^ Material
        {
            MContentElement^ get() { return MContentManager::GetMContentElement(this->sceneElement->GetMaterial()); }
            void set(MContentElement^ value) { this->sceneElement->MaterialID = (value != nullptr ? value->ID : 0); OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Base")]
        property bool Visible
        {
            bool get() { return this->sceneElement->Visible; }
            void set(bool value) { this->sceneElement->Visible = value; OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Transform")]
        property MPoint Position
        {
            MPoint get() { return MPoint(this->sceneElement->Position); }
            void set(MPoint value) { this->sceneElement->Position = value.ToVector3(); OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Transform")]
        property MPoint Rotation
        {
            MPoint get() { return MPoint(this->sceneElement->Rotation.toEulerAngle()); }
            void set(MPoint value) { this->sceneElement->Rotation = Quaternion(value.ToVector3()); OnChanged(); }
        }
        
        [MPropertyAttribute(Group = "Transform")]
        property MPoint Scale
        {
            MPoint get() { return MPoint(this->sceneElement->Scale); }
            void set(MPoint value) { this->sceneElement->Scale = value.ToVector3(); OnChanged(); }
        }
#pragma endregion

        [MPropertyAttribute(Group = "Textures", Choosable = true)]
        property MContentElement^ DiffuseMap
        {
            MContentElement^ get() { return MContentManager::GetMContentElement(this->sceneElement->GetDiffuseMap()); }
            void set(MContentElement^ value) { this->sceneElement->Textures.DiffuseMapID = (value != nullptr ? value->ID : 0); OnChanged(); }
        }

        [MPropertyAttribute(Group = "Textures", Choosable = true)]
        property MContentElement^ NormalMap
        {
            MContentElement^ get() { return MContentManager::GetMContentElement(this->sceneElement->GetNormalMap()); }
            void set(MContentElement^ value) { this->sceneElement->Textures.NormalMapID = (value != nullptr ? value->ID : 0); OnChanged(); }
        }


		delegate void ChangedEventHandler(MSceneElement^ sender);
		event ChangedEventHandler^ Changed;

	public:
		MSceneElement(SceneManager* owner, uint id)
		{
			this->owner = owner;
			this->id = id;
        }


#pragma region SceneElement Functions
#pragma endregion


		void OnChanged()
		{
			this->Changed(this);
		}


		virtual String^ ToString() override
		{
            if (!this->owner->ContainsElement(this->id) || !this->sceneElement)
				return nullptr;

			return this->Type.ToString() + ": " + this->Name + " (" + this->ID + ")";
		}

		virtual bool Equals(Object^ obj) override
		{
            if (!this->owner->ContainsElement(this->id) || !this->sceneElement)
				return false;

			MSceneElement^ mse = dynamic_cast<MSceneElement^>(obj);
			if (mse == nullptr ||
                !this->owner->ContainsElement(mse->id) || !mse->sceneElement)
				return false;
			return this->ID.Equals(mse->ID);
		}

	};

}
