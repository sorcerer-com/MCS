// MContentElement.h
#pragma once

#include "Engine\Content Elements\ContentElement.h"
#include "Engine\Managers\ContentManager.h"
#pragma managed

#include "..\Utils\MHeader.h"


namespace MyEngine {

	public enum class EContentElementType
	{
		Mesh,
		Material,
		Texture,
		UIScreen,
		Skeleton,
		Sound
	};

	public ref class MContentElement
	{
	protected:
		ContentManager* owner;
		uint id;

		property ContentElement* element
		{
			ContentElement* get() { return this->owner->GetElement(this->id, false).get(); }
		}

	public:
		property uint Version
		{
			uint get() { return element->Version; }
		}

		[MPropertyAttribute(Group = "Base")]
		property EContentElementType Type
		{
			EContentElementType get() { return (EContentElementType)element->Type; }
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

		property String^ Package
		{
			String^ get() { return gcnew String(element->Package.c_str()); }
		}

		property String^ Path
		{
			String^ get() { return gcnew String(element->Path.c_str()); }
		}


		[MPropertyAttribute(Group = "Base")]
		property String^ FullPath
		{
			String^ get() { return gcnew String(element->GetFullPath().c_str()); }
		}

		property String^ FullName
		{
			String^ get() { return gcnew String(element->GetFullName().c_str()); }
		}


		[MPropertyAttribute(Group = "Base")]
		property long Size
		{
			long get() { return this->IsLoaded ? (long)element->Size() : 0; }
		}

		[MPropertyAttribute(Group = "Base")]
		property long IsLoaded
		{
			long get() { return (long)element->IsLoaded; }
		}


		property String^ Info
		{
			String^ get()
			{
				return this->Type.ToString() +
					"\nID: " + this->ID.ToString() +
					"\nName: " + this->Name + "#" + this->Version +
					"\nPackage: " + this->Package +
					"\nPath: " + this->Path +
					"\nSize: " + this->Size.ToString("### ### ###");
			}
		}


		delegate void ChangedEventHandler(MContentElement^ sender);
		event ChangedEventHandler^ Changed;

	public:
		MContentElement(ContentManager* owner, uint id)
		{
			this->owner = owner;
			this->id = id;
		}


		virtual bool LoadFromFile(String^)
		{
			return false;
		}

		virtual bool SaveToFile(String^)
		{
			return false;
		}

		void OnChanged()
		{
			this->Changed(this);
		}
		

		virtual String^ ToString() override
		{
			if (!this->owner->ContainsElement(this->id) || !this->element)
				return nullptr;

			return this->Type.ToString() + ": " + this->FullName + " (" + this->ID + ")";
		}

		virtual bool Equals(Object^ obj) override
		{
			if (!this->owner->ContainsElement(this->id) || !this->element)
				return false;

			MContentElement^ elem = dynamic_cast<MContentElement^>(obj);
			if (elem == nullptr || 
				!this->owner->ContainsElement(elem->id) || !elem->element)
				return false;
			return this->ID.Equals(elem->ID);
		}

	};
}

