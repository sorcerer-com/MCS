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

	public ref class MContentElement : NotifyPropertyChanged
	{
	protected:
		ContentManager* owner;
		uint id;

		property ContentElement* element
		{
			ContentElement* get() { return this->owner->GetElement(this->id, false); }
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

		property String^ FullName
		{
			String^ get() { return gcnew String(element->GetFullName().c_str()); }
		}

		property long Size
		{
			long get() { return (long)element->Size(); }
		}


		property String^ Image
		{
			String^ get()
			{
				if (this->Type == EContentElementType::Mesh)
					return  "/Images/ContentWindow/Mesh.png";
				else if (this->Type == EContentElementType::Material)
					return  "/Images/ContentWindow/Material.png";
				else if (this->Type == EContentElementType::Texture)
					return  "/Images/ContentWindow/Texture.png";
				else if (this->Type == EContentElementType::UIScreen)
					return  "/Images/ContentWindow/UIScreen.png";
				else if (this->Type == EContentElementType::Skeleton)
					return  "/Images/ContentWindow/Skeleton.png";
				else if (this->Type == EContentElementType::Sound)
					return  "/Images/ContentWindow/Sound.png";
				return "";
			}
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
			return this->Type.ToString() + ": " + this->FullName + " (" + this->ID + ")";
		}

		virtual bool Equals(Object^ obj) override
		{
			MContentElement^ elem = dynamic_cast<MContentElement^>(obj);
			if (elem == nullptr)
				return false;
			return this->ID.Equals(elem->ID);
		}


		static int CompareByName(MContentElement^ elem1, MContentElement^ elem2)
		{
			return elem1->Name->CompareTo(elem2->Name);
		}
	};
}

