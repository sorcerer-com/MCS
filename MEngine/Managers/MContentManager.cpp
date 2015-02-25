// MContentManager.cpp

#include "stdafx.h"
#include "MContentManager.h"

#include "..\Content Elements\MContentElement.h"
#include "..\Content Elements\MMesh.h"


namespace MyEngine {
	
	List<String^>^ MContentManager::Paths::get()
	{
		List<String^>^ collection = gcnew List<String^>();

		auto paths = this->contentManager->GetPaths();
		for (const auto& path : paths)
			collection->Add(gcnew String(path.c_str()));

		return collection;
	}

	List<MContentElement^>^ MContentManager::Content::get()
	{
		List<MContentElement^>^ collection = gcnew List<MContentElement^>();

		auto elements = this->contentManager->GetElements();
		for (const auto& element : elements)
			collection->Add(MContentManager::getMContentElement(element));

		return collection;
	}


	MContentManager::MContentManager(ContentManager* contentManager)
	{
		this->contentManager = contentManager;
	}


	bool MContentManager::ImportPackage(String^ filePath)
	{
		bool res = this->contentManager->ImportPackage(to_string(filePath));
		if (res)
			this->OnChanged(nullptr);
		return res;
	}

	bool MContentManager::ExportToPackage(String^ filePath, uint id)
	{
		return this->contentManager->ExportToPackage(to_string(filePath), id);
	}


	bool MContentManager::CreatePath(String^ fullPath)
	{
		bool res = this->contentManager->CreatePath(to_string(fullPath));
		if (res)
			this->OnChanged(nullptr);
		return res;
	}

	bool MContentManager::RenamePath(String^ oldFullPath, String^ newFullPath)
	{
		bool res = this->contentManager->RenamePath(to_string(oldFullPath), to_string(newFullPath));
		if (res)
			this->OnChanged(nullptr);
		return res;
	}

	bool MContentManager::ContainPath(String^ fullPath)
	{
		return this->contentManager->ContainPath(to_string(fullPath));
	}

	bool MContentManager::DeletePath(String^ fullPath)
	{
		bool res = this->contentManager->DeletePath(to_string(fullPath));
		if (res)
			this->OnChanged(nullptr);
		return res;
	}


	MContentElement^ MContentManager::AddElement(EContentElementType type, String^ name, String^ package, String^ path, uint id)
	{
		ContentElementPtr elem = this->contentManager->AddElement((ContentElementType)type, to_string(name), to_string(package), to_string(path), id);
		MContentElement^ mce = this->getMContentElement(elem);
		if (mce != nullptr)
			this->OnChanged(mce);
		return mce;
	}

	MContentElement^ MContentManager::CloneElement(uint id, String^ newName)
	{
		ContentElementPtr elem = this->contentManager->GetElement(id, true, true);
		if (!elem)
			return nullptr;

		ContentElement* newElem = elem->Clone();
		newElem->ID = 0;
		newElem->Name = to_string(newName);

		if (this->contentManager->AddElement(newElem))
		{
			MContentElement^ mce = this->getMContentElement(this->contentManager->GetElement(newElem->ID, false));
			this->OnChanged(mce);
			return mce;
		}
		return nullptr;
	}

	bool MContentManager::ContainElement(uint id)
	{
		return this->contentManager->ContainElement(id);
	}

	bool MContentManager::ContainElement(String^ fullName)
	{
		return this->contentManager->ContainElement(to_string(fullName));
	}

	bool MContentManager::RenameElement(uint id, String^ newName)
	{
		ContentElementPtr elem = this->contentManager->GetElement(id, true, true);
		if (!elem)
			return false;

		string newFullName = elem->GetFullPath() + to_string(newName);
		if (this->contentManager->GetElement(newFullName, false))
			return false;

		elem->Name = to_string(newName);
		this->contentManager->SaveElement(elem->ID);

		this->OnChanged(this->getMContentElement(elem));
		return true;
	}

	bool MContentManager::MoveElement(uint id, String^ newFullPath)
	{
		bool res = this->contentManager->MoveElement(id, to_string(newFullPath));
		if (res)
			this->OnChanged(this->getMContentElement(this->contentManager->GetElement(id, true, true)));
		return res;
	}

	bool MContentManager::DeleteElement(uint id)
	{
		bool res = this->contentManager->DeleteElement(id);
		if (res)
			this->OnChanged(nullptr);
		return res;
	}

	MContentElement^ MContentManager::GetElement(uint id)
	{
		return this->GetElement(id, true);
	}

	MContentElement^ MContentManager::GetElement(uint id, bool load)
	{
		if (!this->contentManager->ContainElement(id))
			return nullptr;

		ContentElementPtr elem = this->contentManager->GetElement(id, load, load);
		return this->getMContentElement(elem);
	}

	MContentElement^ MContentManager::GetElement(String^ fullName)
	{
		return this->GetElement(fullName, true);
	}
	
	MContentElement^ MContentManager::GetElement(String^ fullName, bool load)
	{
		ContentElementPtr elem = this->contentManager->GetElement(to_string(fullName), load, load);
		return this->getMContentElement(elem);
	}

	void MContentManager::SaveElement(uint id)
	{
		this->contentManager->SaveElement(id);
	}


	void MContentManager::OnChanged(MContentElement^ element)
	{
		this->Changed(this, element);
	}

	void MContentManager::OnElementChanged(MContentElement^ sender)
	{
		this->OnChanged(sender);
	}


	MContentElement^ MContentManager::getMContentElement(const ContentElementPtr& element)
	{
		if (!element)
			return nullptr;

		if (!element->IsLoaded)
			return gcnew MContentElement(element->Owner, element->ID);

		MContentElement^ melement = nullptr;
		if (element->Type == ContentElementType::EMesh)
			melement = gcnew MMesh(element->Owner, element->ID);
		/* TODO: add content elements
		else if (element->Type == EMaterial)
			melement = gcnew MMaterial(element->GetOwner(), element->ID);
		else if (element->Type == ETexture)
			melement = gcnew MTexture(element->GetOwner(), element->ID);
		else if (element->Type == EUIScreen)
			melement = gcnew MUIScreen(element->GetOwner(), element->ID);
		else if (element->Type == ESkeleton)
			melement = gcnew MSkeleton(element->GetOwner(), element->ID);
		else if (element->Type == ESound)
			melement = gcnew MSound(element->GetOwner(), element->ID);
			*/
		return melement;
	}


	String^ MContentManager::GetPackage(String^ fullName)
	{
		return gcnew String(ContentManager::GetPackage(to_string(fullName)).c_str());
	}

	String^ MContentManager::GetPath(String^ fullName)
	{
		return gcnew String(ContentManager::GetPath(to_string(fullName)).c_str());
	}

	String^ MContentManager::GetName(String^ fullName)
	{
		return gcnew String(ContentManager::GetName(to_string(fullName)).c_str());
	}

}