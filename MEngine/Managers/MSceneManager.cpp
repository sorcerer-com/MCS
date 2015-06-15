// MSceneManager.cpp

#include "stdafx.h"
#include "MSceneManager.h"

#include "..\MEngine.h"
#include "..\Utils\Types\MColor.h"
#include "..\Scene Elements\MSceneElement.h"
#include "..\Scene Elements\MCamera.h"
#include "..\Scene Elements\MLight.h"


namespace MyEngine {

	List<MSceneElement^>^ MSceneManager::Elements::get()
	{
		List<MSceneElement^>^ collection = gcnew List<MSceneElement^>();

		const auto elements = this->sceneManager->GetElements();
		for (const auto& element : elements)
			collection->Add(this->getMSceneElement(element));

		return collection;
	}
	
	List<String^>^ MSceneManager::Layers::get()
	{
		List<String^>^ collection = gcnew List<String^>();

		const auto layers = this->sceneManager->GetLayers();
		for (const auto& layer : layers)
			collection->Add(gcnew String(layer.c_str()));

		return collection;
	}
	
	MCamera^ MSceneManager::ActiveCamera::get()
	{
		if (this->sceneManager->ActiveCamera)
		{
			MCamera^ camera = gcnew MCamera(this->sceneManager, this->sceneManager->ActiveCamera->ID);
			camera->Changed += gcnew MSceneElement::ChangedEventHandler(this, &MSceneManager::OnElementChanged);
			return camera;
		}

		return nullptr;
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

	MColor MSceneManager::FogColor::get()
	{
		return MColor(this->sceneManager->FogColor);
	}

	void MSceneManager::FogColor::set(MColor value)
	{
		this->sceneManager->FogColor = value.ToColor4();
		OnChanged(nullptr);
	}

	double MSceneManager::FogDensity::get()
	{
		return this->sceneManager->FogDensity;
	}

	void MSceneManager::FogDensity::set(double value)
	{
		this->sceneManager->FogDensity = (float)value;
		OnChanged(nullptr);
    }

    double MSceneManager::TimeOfDay::get()
    {
        return this->sceneManager->TimeOfDay;
    }

    void MSceneManager::TimeOfDay::set(double value)
    {
        this->sceneManager->SetTimeOfDay((float)value);
        OnChanged(nullptr);
    }

    MContentElement^ MSceneManager::SkyBox::get()
    {
        return this->owner->ContentManager->GetElement(this->sceneManager->SkyBox);
    }

    void MSceneManager::SkyBox::set(MContentElement^ value)
    {
        this->sceneManager->SetSkyBox((value != nullptr ? value->ID : 0));
        OnChanged(nullptr);
    }


    MSceneManager::MSceneManager(MEngine^ owner, SceneManager* sceneManager) : 
        MBaseManager(owner)
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
			this->OnChanged(mse);
		return mse;
	}

	MSceneElement^ MSceneManager::AddElement(ESceneElementType type, String^ name, String^ contentFullName)
	{
		SceneElementPtr elem = this->sceneManager->AddElement((SceneElementType)type, to_string(name), to_string(contentFullName));
		MSceneElement^ mse = this->getMSceneElement(elem);
		if (mse != nullptr)
			this->OnChanged(mse);
		return mse;
	}

	MSceneElement^ MSceneManager::CloneElement(uint id, String^ newName)
	{
		SceneElementPtr elem = this->sceneManager->GetElement(id);
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

	bool MSceneManager::ContainsElement(uint id)
	{
		return this->sceneManager->ContainsElement(id);
	}

	bool MSceneManager::ContainsElement(String^ name)
	{
		return this->sceneManager->ContainsElement(to_string(name));
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
	
	bool MSceneManager::SetElementLayer(uint id, String^ layer)
	{
		SceneElementPtr elem = this->sceneManager->GetElement(id);
		if (!elem)
			return false;

		if (!this->sceneManager->ContainsLayer(to_string(layer)))
			return false;

		elem->Layer = to_string(layer);
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
		if (!this->sceneManager->ContainsElement(id))
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
    

	bool MSceneManager::CreateLayer(String^ layer)
	{
		bool res = this->sceneManager->CreateLayer(to_string(layer));
		if (res)
			this->OnChanged(nullptr);
		return res;
	}

	bool MSceneManager::RenameLayer(String^ oldLayer, String^ newLayer)
	{
		bool res = this->sceneManager->RenameLayer(to_string(oldLayer), to_string(newLayer));
		if (res)
			this->OnChanged(nullptr);
		return res;
	}

	bool MSceneManager::ContainsLayer(String^ layer)
	{
		return this->sceneManager->ContainsLayer(to_string(layer));
	}

	bool MSceneManager::DeleteLayer(String^ layer)
	{
		bool res = this->sceneManager->DeleteLayer(to_string(layer));
		if (res)
			this->OnChanged(nullptr);
		return res;
	}
	
	List<MSceneElement^>^ MSceneManager::GetLayerElements(String^ layer)
	{
		List<MSceneElement^>^ collection = gcnew List<MSceneElement^>();

		const auto elements = this->sceneManager->GetLayerElements(to_string(layer));
		for (const auto& element : elements)
			collection->Add(this->getMSceneElement(element));

		return collection;
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
		MSceneElement^ mse = MSceneManager::GetMSceneElement(element);

		if (mse != nullptr)
			mse->Changed += gcnew MSceneElement::ChangedEventHandler(this, &MSceneManager::OnElementChanged);
		return mse;
	}


	MSceneElement^ MSceneManager::GetMSceneElement(const SceneElementPtr& element)
	{
		if (!element)
			return nullptr;

		MSceneElement^ mse = nullptr;
		if (element->Type == SceneElementType::ECamera)
			mse = gcnew MCamera(element->Owner, element->ID);
		else if (element->Type == SceneElementType::ELight)
			mse = gcnew MLight(element->Owner, element->ID);
		/* TODO: add scene elements */
		else
			mse = gcnew MSceneElement(element->Owner, element->ID);

		return mse;
	}

}