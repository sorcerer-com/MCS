// MContentManager.cpp

#include "stdafx.h"
#include "MContentManager.h"


namespace MEngine {

	// TODO: this->OnPropertyChanged("Content");

	MContentManager::MContentManager(ContentManager* contentManager)
	{
		this->contentManager = contentManager;
	}


	bool MContentManager::ImportPackage(String^ filePath)
	{
		return this->contentManager->ImportPackage(to_string(filePath));
	}

	bool MContentManager::ExportToPackage(String^ filePath, uint id)
	{
		return this->contentManager->ExportToPackage(to_string(filePath), id);
	}


	bool MContentManager::CreatePath(String^ fullPath)
	{
		return this->contentManager->CreatePath(to_string(fullPath));
	}

	bool MContentManager::RenamePath(String^ oldFullPath, String^ newFullPath)
	{
		return this->contentManager->RenamePath(to_string(oldFullPath), to_string(newFullPath));
	}

	bool MContentManager::DeletePath(String^ fullPath)
	{
		return this->contentManager->DeletePath(to_string(fullPath));
	}


	MContentElement^ MContentManager::AddElement(EContentElementType type, String^ name, String^ package, String^ path, uint id)
	{
		ContentElement* elem = this->contentManager->AddElement((ContentElementType)type, to_string(name), to_string(package), to_string(path), id);
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
		return true;
	}

	bool MContentManager::MoveElement(uint id, String^ newFullPath)
	{
		bool res = this->contentManager->MoveElement(id, to_string(newFullPath));
		return res;
	}

	bool MContentManager::DeleteElement(uint id)
	{
		bool res = this->contentManager->DeleteElement(id);
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