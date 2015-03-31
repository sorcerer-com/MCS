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
		SystemObject,
		StaticObject,
	};

	public ref class MSceneElement
	{
	protected:
		SceneManager* owner;
		uint id;

		property SceneElement* element
		{
			SceneElement* get() { return this->owner->GetElement(this->id).get(); }
		}

	public:
		property uint Version
		{
			uint get() { return element->Version; }
		}

		[MPropertyAttribute(Group = "Base")]
		property ESceneElementType Type
		{
			ESceneElementType get() { return (ESceneElementType)element->Type; }
		}

		[MPropertyAttribute(Group = "Base")]
		property uint ID
		{
			uint get() { return element->ID; }
		}

		[MPropertyAttribute(Group = "Base")]
		property String^ Name
		{
			String^ get() { return gcnew String(element->Name.c_str()); }
		}

		[MPropertyAttribute(Group = "Base")]
		property String^ Layer
		{
			String^ get() { return gcnew String(element->Layer.c_str()); }
		}

		[MPropertyAttribute(Group = "Content", Choosable = true)]
		property MContentElement^ Content
		{
			MContentElement^ get() { return MContentManager::GetMContentElement(this->element->GetContent()); }
			void set(MContentElement^ value) { if (value != nullptr) element->ContentID = value->ID; else element->ContentID = 0; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Content", Choosable = true)]
		property MContentElement^ Material
		{
			MContentElement^ get() { return MContentManager::GetMContentElement(this->element->GetMaterial()); }
			void set(MContentElement^ value) { if (value != nullptr) element->MaterialID = value->ID; else element->MaterialID = 0; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Base")]
		property bool Visible
		{
			bool get() { return element->Visible; }
			void set(bool value) { element->Visible = value; OnChanged(); }
		}

		[MPropertyAttribute(Group = "Transform")]
		property MPoint Position
		{
			MPoint get() { return MPoint(element->Position); }
			void set(MPoint value) { element->Position = value.ToVector3(); OnChanged(); }
		}

		[MPropertyAttribute(Group = "Transform")]
		property MPoint Rotation
		{
			MPoint get() { return MPoint(element->Rotation.toEulerAngle()); }
			void set(MPoint value) { element->Rotation = value.ToVector3(); OnChanged(); }
		}

		[MPropertyAttribute(Group = "Transform")]
		property MPoint Scale
		{
			MPoint get() { return MPoint(element->Scale); }
			void set(MPoint value) { element->Scale = value.ToVector3(); OnChanged(); }
		}


		// TODO: add EventArgs that describes what is changed, and may be sender to object
		delegate void ChangedEventHandler(MSceneElement^ sender);
		event ChangedEventHandler^ Changed;

	public:
		MSceneElement(SceneManager* owner, uint id)
		{
			this->owner = owner;
			this->id = id;
		}


		void OnChanged()
		{
			this->Changed(this);
		}


		virtual String^ ToString() override
		{
			if (!this->owner->ContainElement(this->id) || !this->element)
				return nullptr;

			return this->Type.ToString() + ": " + this->Name + " (" + this->ID + ")";
		}

		virtual bool Equals(Object^ obj) override
		{
			if (!this->owner->ContainElement(this->id) || !this->element)
				return false;

			MSceneElement^ mse = dynamic_cast<MSceneElement^>(obj);
			if (mse == nullptr ||
				!this->owner->ContainElement(mse->id) || !mse->element)
				return false;
			return this->ID.Equals(mse->ID);
		}

	};

}