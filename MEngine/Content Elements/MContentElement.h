// MContentElement.h
#pragma once

#include "Engine\Content Elements\ContentElement.h"
#include "Engine\Managers\ContentManager.h"
#pragma managed

#include "..\Utils\MHeader.h"


namespace MEngine {

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

		property EContentElementType Type
		{
			EContentElementType get() { return (EContentElementType)element->Type; }
		}

		property uint ID
		{
			uint get() { return element->ID; }
		}

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


		property String^ FullPath
		{
			String^ get() { return gcnew String(element->GetFullPath().c_str()); }
		}

		property String^ FullName
		{
			String^ get() { return gcnew String(element->GetFullName().c_str()); }
		}


		property long Size
		{
			long get() { return this->IsLoaded ? (long)element->Size() : 0; }
		}

		property long IsLoaded
		{
			long get() { return (long)element->IsLoaded; }
		}

		
		property String^ Info
		{
			String^ get()
			{
				return this->Type.ToString() +
					"\nID: " + this->ID +
					"\nName: " + this->Name + "#" + this->Version +
					"\nPackage: " + this->Package +
					"\nPath: " + this->Path +
					"\nSize: " + this->Size.ToString("### ### ###");
			}
		}

	public:
		MContentElement(ContentManager* owner, uint id)
		{
			this->owner = owner;
			this->id = id;
		}
		

		virtual String^ ToString() override
		{
			if (!this->owner->ContainElement(this->id) || !this->element)
				return nullptr;

			return this->Type.ToString() + ": " + this->FullName + " (" + this->ID + ")";
		}

		virtual bool Equals(Object^ obj) override
		{
			if (!this->owner->ContainElement(this->id) || !this->element)
				return false;

			MContentElement^ elem = dynamic_cast<MContentElement^>(obj);
			if (elem == nullptr || 
				!this->owner->ContainElement(elem->id) || !elem->element)
				return false;
			return this->ID.Equals(elem->ID);
		}



		static String^ GetPackage(String^ fullName)
		{
			return gcnew String(ContentElement::GetPackage(to_string(fullName)).c_str());
		}

		static String^ GetPath(String^ fullName)
		{
			return gcnew String(ContentElement::GetPath(to_string(fullName)).c_str());
		}

		static String^ GetName(String^ fullName)
		{
			return gcnew String(ContentElement::GetName(to_string(fullName)).c_str());
		}
	};
}

