// MContentManager.cpp

#include "stdafx.h"
#include "MContentManager.h"


namespace MEngine {
	
	List<String^>^ MContentManager::Paths::get()
	{
		List<String^>^ collection = gcnew List<String^>();

		auto paths = this->contentManager->GetPaths();
		for (auto& path : paths)
			collection->Add(gcnew String(path.c_str()));

		return collection;
	}

	List<MContentElement^>^ MContentManager::Content::get()
	{
		List<MContentElement^>^ collection = gcnew List<MContentElement^>();

		auto elements = this->contentManager->GetElements();
		for (auto& element : elements)
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
		this->OnPropertyChanged("Paths");
		this->OnPropertyChanged("Content");
		return res;
	}

	bool MContentManager::ExportToPackage(String^ filePath, uint id)
	{
		return this->contentManager->ExportToPackage(to_string(filePath), id);
	}


	bool MContentManager::CreatePath(String^ fullPath)
	{
		bool res = this->contentManager->CreatePath(to_string(fullPath));
		this->OnPropertyChanged("Paths");
		return res;
	}

	bool MContentManager::RenamePath(String^ oldFullPath, String^ newFullPath)
	{
		bool res = this->contentManager->RenamePath(to_string(oldFullPath), to_string(newFullPath));
		this->OnPropertyChanged("Paths");
		return res;
	}

	bool MContentManager::DeletePath(String^ fullPath)
	{
		bool res = this->contentManager->DeletePath(to_string(fullPath));
		this->OnPropertyChanged("Paths");
		return res;
	}


	MContentElement^ MContentManager::AddElement(EContentElementType type, String^ name, String^ package, String^ path, uint id)
	{
		ContentElement* elem = this->contentManager->AddElement((ContentElementType)type, to_string(name), to_string(package), to_string(path), id);
		
		this->OnPropertyChanged("Paths");
		this->OnPropertyChanged("Content");
		return this->getMContentElement(elem);
	}

	MContentElement^ MContentManager::CloneElement(uint id, String^ newName)
	{
		ContentElement* elem = this->contentManager->GetElement(id, true);
		if (elem == NULL)
			return nullptr;

		elem = elem->Clone();
		elem->ID = 0;
		elem->Name = to_string(newName);
		this->contentManager->AddElement(elem);
		
		this->OnPropertyChanged("Content");
		return this->getMContentElement(elem);
	}

	bool MContentManager::RenameElement(uint id, String^ newName)
	{
		ContentElement* elem = this->contentManager->GetElement(id, true);
		if (elem == NULL)
			return false;

		string path = elem->Path + to_string(newName);
		if (this->contentManager->GetElement(path, false) != NULL)
			return false;

		elem->Name = to_string(newName);
		this->OnPropertyChanged("Content");
		return true;
	}

	bool MContentManager::MoveElement(uint id, String^ newFullPath)
	{
		bool res = this->contentManager->MoveElement(id, to_string(newFullPath));

		this->OnPropertyChanged("Paths");
		this->OnPropertyChanged("Content");
		return res;
	}

	bool MContentManager::DeleteElement(uint id)
	{
		bool res = this->contentManager->DeleteElement(id);
		this->OnPropertyChanged("Content");
		return res;
	}

	MContentElement^ MContentManager::GetElement(uint id)
	{
		if (!this->contentManager->ContainElement(id))
			return nullptr;

		ContentElement* elem = this->contentManager->GetElement(id, true);
		return this->getMContentElement(elem);
	}

	MContentElement^ MContentManager::GetElement(String^ fullName)
	{
		ContentElement* elem = this->contentManager->GetElement(to_string(fullName), true);
		return this->getMContentElement(elem);
	}

	void MContentManager::SaveElement(uint id)
	{
		this->contentManager->SaveElement(id);
	}


	MContentElement^ MContentManager::getMContentElement(ContentElement* element)
	{
		if (element == NULL)
			return nullptr;

		MContentElement^ melement = nullptr;
		if (!element->IsLoaded)
			return gcnew MContentElement(element->Owner, element->ID);
		/* TODO: add content elements 
		if (element->Type == EMesh)
			melement = gcnew MMesh(element->GetOwner(), element->ID);
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

}