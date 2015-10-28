// MSceneManager.cpp

#include "stdafx.h"
#include "MSceneManager.h"

#include "..\MEngine.h"
#include "..\Utils\Types\MColor.h"
#include "..\Scene Elements\MSceneElement.h"
#include "..\Scene Elements\MRenderElement.h"
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

#pragma region SceneManager Properties_cpp
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
#pragma endregion

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
        return this->owner->ContentManager->GetElement(this->sceneManager->SkyBoxID);
    }

    void MSceneManager::SkyBox::set(MContentElement^ value)
    {
        this->sceneManager->SetSkyBox((value != nullptr ? value->ID : 0));
        OnChanged(nullptr);
    }

    List<byte>^ MSceneManager::Script::get()
    {
        List<byte>^ collection = gcnew List<byte>();
        const auto& res = this->sceneManager->Script;
        for (const auto& value : res)
            collection->Add(value);
        return collection;
    }

    void MSceneManager::Script::set(List<byte>^ value)
    {
        this->sceneManager->Script.resize(value->Count);
        for (int i = 0; i < value->Count; i++)
            this->sceneManager->Script[i] = value[i];
        OnChanged(nullptr);
    }


    MSceneManager::MSceneManager(MEngine^ owner, SceneManager* sceneManager) : 
        MBaseManager(owner)
	{
		this->sceneManager = sceneManager;
	}


#pragma region SceneManager Functions_cpp
	void MSceneManager::New()
	{
	    this->sceneManager->New();
	    this->OnChanged(nullptr);
	}
	
	bool MSceneManager::Save(String^ filePath)
	{
	    return this->sceneManager->Save(to_string(filePath));
	}
	
	bool MSceneManager::Load(String^ filePath)
	{
	    bool res = this->sceneManager->Load(to_string(filePath));
	    if (res)
	        this->OnChanged(nullptr);
	    return res;
	}
	
	
	MSceneElement^ MSceneManager::AddElement(ESceneElementType type, String^ name, uint contentID, uint id)
	{
	    MSceneElement^ res = this->getMSceneElement(this->sceneManager->AddElement((SceneElementType)type, to_string(name), contentID, id));
	    if (res != nullptr)
	        this->OnChanged(res);
	    return res;
	}
	
	MSceneElement^ MSceneManager::AddElement(ESceneElementType type, String^ name, String^ contentFullName, uint id)
	{
	    MSceneElement^ res = this->getMSceneElement(this->sceneManager->AddElement((SceneElementType)type, to_string(name), to_string(contentFullName), id));
	    if (res != nullptr)
	        this->OnChanged(res);
	    return res;
	}
	
	bool MSceneManager::ContainsElement(uint id)
	{
	    return this->sceneManager->ContainsElement(id);
	}
	
	bool MSceneManager::ContainsElement(String^ name)
	{
	    return this->sceneManager->ContainsElement(to_string(name));
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
	    return this->getMSceneElement(this->sceneManager->GetElement(id));
	}
	
	MSceneElement^ MSceneManager::GetElement(String^ name)
	{
	    return this->getMSceneElement(this->sceneManager->GetElement(to_string(name)));
	}
	
	List<MSceneElement^>^ MSceneManager::GetElements(ESceneElementType type)
	{
	    List<MSceneElement^>^ collection = gcnew List<MSceneElement^>();
	    const auto& res = this->sceneManager->GetElements((SceneElementType)type);
	    for (const auto& value : res)
	        collection->Add(this->getMSceneElement(value));
	    return collection;
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
	    const auto& res = this->sceneManager->GetLayerElements(to_string(layer));
	    for (const auto& value : res)
	        collection->Add(this->getMSceneElement(value));
	    return collection;
	}
#pragma endregion

    MSceneElement^ MSceneManager::AddElement(ESceneElementType type, String^ name, uint contentID)
    {
        return this->AddElement(type, name, contentID, 0);
    }

    MSceneElement^ MSceneManager::AddElement(ESceneElementType type, String^ name, String^ contentFullName)
    {
        return this->AddElement(type, name, contentFullName, 0);
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
        else if (element->Type == SceneElementType::ERenderObject)
            mse = gcnew MRenderElement(element->Owner, element->ID);
		/* TODO: add scene elements */
		else
			mse = gcnew MSceneElement(element->Owner, element->ID);

		return mse;
	}

}
