// MSceneManager.cpp

#include "stdafx.h"
#include "MSceneManager.h"

#include "..\Utils\Types\MColor.h"
#include "..\Scene Elements\MSceneElement.h"
#include "..\Scene Elements\MCamera.h"
#include "..\Scene Elements\MLight.h"


namespace MyEngine {

	List<MSceneElement^>^ MSceneManager::Elements::get()
	{
		List<MSceneElement^>^ collection = gcnew List<MSceneElement^>();

		auto elements = this->sceneManager->GetElements();
		for (const auto& element : elements)
			collection->Add(MSceneManager::getMSceneElement(element));

		return collection;
	}
	
	MCamera^ MSceneManager::ActiveCamera::get()
	{
		return gcnew MCamera(this->sceneManager, this->sceneManager->ActiveCamera->ID);
	}
	
	void MSceneManager::ActiveCamera::set(MCamera^ value)
	{
		if (value != nullptr)
		{
			SceneElementPtr elem = this->sceneManager->GetElement(value->ID);
			if (elem) this->sceneManager->ActiveCamera = (Camera*)elem.get();
			OnChanged(value);
		}
	}

	MColor MSceneManager::AmbientLight::get()
	{
		return MColor(this->sceneManager->AmbientLight);
	}

	void MSceneManager::AmbientLight::set(MColor value)
	{
		this->sceneManager->AmbientLight = value.ToColor4();
		OnChanged(nullptr);
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
		MSceneElement^ mse = this->getMSceneElement(elem);
		if (mse != nullptr)
		{
			mse->Changed += gcnew MSceneElement::ChangedEventHandler(this, &MSceneManager::OnElementChanged);
			this->OnChanged(mse);
		}
		return mse;
	}

	MSceneElement^ MSceneManager::AddElement(ESceneElementType type, String^ name, String^ contentFullName)
	{
		SceneElementPtr elem = this->sceneManager->AddElement((SceneElementType)type, to_string(name), to_string(contentFullName));
		MSceneElement^ mse = this->getMSceneElement(elem);
		if (mse != nullptr)
		{
			mse->Changed += gcnew MSceneElement::ChangedEventHandler(this, &MSceneManager::OnElementChanged);
			this->OnChanged(mse);
		}
		return mse;
	}

	MSceneElement^ MSceneManager::CloneElement(MSceneElement^ element, String^ newName)
	{
		SceneElementPtr elem = this->sceneManager->GetElement(element->ID);
		if (!elem)
			return nullptr;

		SceneElement* newElem = elem->Clone();
		newElem->ID = 0;
		newElem->Name = to_string(newName);

		if (this->sceneManager->AddElement(newElem))
		{
			MSceneElement^ mse = this->getMSceneElement(this->sceneManager->GetElement(newElem->ID));
			this->OnChanged(mse);
			return mse;
		}
		return nullptr;
	}

	bool MSceneManager::ContainElement(uint id)
	{
		return this->sceneManager->ContainElement(id);
	}

	bool MSceneManager::ContainElement(String^ name)
	{
		return this->sceneManager->ContainElement(to_string(name));
	}

	bool MSceneManager::RenameElement(String^ oldName, String^ newName)
	{
		if (this->sceneManager->GetElement(to_string(newName)))
			return false;

		SceneElementPtr elem = this->sceneManager->GetElement(to_string(oldName));
		if (!elem)
			return false;

		elem->Name = to_string(newName);

		this->OnChanged(this->getMSceneElement(elem));
		return true;
	}

	bool MSceneManager::DeleteElement(uint id)
	{
		bool res = this->sceneManager->DeleteElement(id);
		if (res)
			this->OnChanged(nullptr);
		return res;
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


	void MSceneManager::OnChanged(MSceneElement^ element)
	{
		this->Changed(this, element);
	}

	void MSceneManager::OnElementChanged(MSceneElement^ sender)
	{
		this->OnChanged(sender);
	}

	MSceneElement^ MSceneManager::getMSceneElement(const SceneElementPtr& element)
	{
		if (!element)
			return nullptr;

		MSceneElement^ melement = nullptr;
		if (element->Type == ECamera)
			melement = gcnew MCamera(element->Owner, element->ID);
		else if (element->Type == ELight)
			melement = gcnew MLight(element->Owner, element->ID);
		/* TODO: add scene elements
		else if (element->Type == ECharacter)
			melement = gcnew MCharacter(element->GetOwner(), element->ID); */
		else
			melement = gcnew MSceneElement(element->Owner, element->ID);

		return melement;
	}

}