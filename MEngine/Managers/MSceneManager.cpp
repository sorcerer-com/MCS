// MSceneManager.cpp

#include "stdafx.h"
#include "MSceneManager.h"

#include "..\Scene Elements\MSceneElement.h"


namespace MyEngine {

	List<MSceneElement^>^ MSceneManager::Elements::get()
	{
		List<MSceneElement^>^ collection = gcnew List<MSceneElement^>();

		auto elements = this->sceneManager->GetElements();
		for (const auto& element : elements)
			collection->Add(MSceneManager::getMSceneElement(element));

		return collection;
	}


	MSceneManager::MSceneManager(SceneManager* sceneManager)
	{
		this->sceneManager = sceneManager;
	}


	void MSceneManager::New()
	{
		this->sceneManager->New();
	}

	bool MSceneManager::Load(String^ filePath)
	{
		return this->sceneManager->Load(to_string(filePath));
	}

	bool MSceneManager::Save(String^ filePath)
	{
		return this->sceneManager->Save(to_string(filePath));
	}


	MSceneElement^ MSceneManager::AddElement(ESceneElementType type, String^ name, uint contentID)
	{
		SceneElementPtr elem = this->sceneManager->AddElement((SceneElementType)type, to_string(name), contentID);
		return this->getMSceneElement(elem);
	}

	MSceneElement^ MSceneManager::AddElement(ESceneElementType type, String^ name, String^ contentFullName)
	{
		SceneElementPtr elem = this->sceneManager->AddElement((SceneElementType)type, to_string(name), to_string(contentFullName));
		return this->getMSceneElement(elem);
	}

	MSceneElement^ MSceneManager::CloneElement(MSceneElement^ element, String^ newName)
	{
		SceneElementPtr elem = this->sceneManager->GetElement(element->ID);
		if (!elem)
			return nullptr;

		SceneElement* newElem = elem->Clone();
		newElem->ID = 0;
		newElem->Name = to_string(newName);
		this->sceneManager->AddElement(newElem);

		return this->getMSceneElement(this->sceneManager->GetElement(newElem->ID));
	}

	bool MSceneManager::ContainElement(uint id)
	{
		return this->sceneManager->ContainElement(id);
	}

	bool MSceneManager::RenameElement(String^ oldName, String^ newName)
	{
		if (this->sceneManager->GetElement(to_string(newName)))
			return false;

		SceneElementPtr elem = this->sceneManager->GetElement(to_string(oldName));
		if (!elem)
			return false;

		elem->Name = to_string(newName);
		return true;
	}

	bool MSceneManager::DeleteElement(uint id)
	{
		return this->sceneManager->DeleteElement(id);
	}

	MSceneElement^ MSceneManager::GetElement(uint id)
	{
		if (!this->sceneManager->ContainElement(id))
			return nullptr;

		SceneElementPtr elem = this->sceneManager->GetElement(id);
		return this->getMSceneElement(elem);
	}

	MSceneElement^ MSceneManager::GetElement(String^ name)
	{
		SceneElementPtr elem = this->sceneManager->GetElement(to_string(name));
		if (!elem)
			return nullptr;

		return this->getMSceneElement(elem);
	}


	MSceneElement^ MSceneManager::getMSceneElement(const SceneElementPtr& element)
	{
		if (!element)
			return nullptr;

		MSceneElement^ melement = nullptr;
		/* TODO: add scene elements
		if (element->Type == ELight)
			melement = gcnew MLight(element->GetOwner(), element->ID);
		else if (element->Type == ECharacter)
			melement = gcnew MCharacter(element->GetOwner(), element->ID);
		else */
			melement = gcnew MSceneElement(element->Owner, element->ID);

		return melement;
	}

}