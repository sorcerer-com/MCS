// MContentManager.cpp

#include "stdafx.h"
#include "MContentManager.h"

#include "..\Content Elements\MContentElement.h"
#include "..\Content Elements\MMesh.h"
#include "..\Content Elements\MMaterial.h"
#include "..\Content Elements\MTexture.h"


namespace MyEngine {
	
	List<String^>^ MContentManager::Paths::get()
	{
		List<String^>^ collection = gcnew List<String^>();

		const auto paths = this->contentManager->GetPaths();
		for (const auto& path : paths)
			collection->Add(gcnew String(path.c_str()));

		return collection;
	}

	List<MContentElement^>^ MContentManager::Content::get()
	{
		List<MContentElement^>^ collection = gcnew List<MContentElement^>();

		const auto elements = this->contentManager->GetElements();
		for (const auto& element : elements)
			collection->Add(this->getMContentElement(element));

		return collection;
    }

#pragma region ContentManager Properties_cpp
#pragma endregion


    MContentManager::MContentManager(MEngine^ owner, ContentManager* contentManager) :
        MBaseManager(owner)
	{
		this->contentManager = contentManager;
	}


#pragma region ContentManager Functions_cpp
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
	
	bool MContentManager::ContainsPath(String^ fullPath)
	{
	    return this->contentManager->ContainsPath(to_string(fullPath));
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
	    MContentElement^ res = this->getMContentElement(this->contentManager->AddElement((ContentElementType)type, to_string(name), to_string(package), to_string(path), id));
	    if (res != nullptr)
	        this->OnChanged(res);
	    return res;
	}
	
	bool MContentManager::ContainsElement(uint id)
	{
	    return this->contentManager->ContainsElement(id);
	}
	
	bool MContentManager::ContainsElement(String^ fullName)
	{
	    return this->contentManager->ContainsElement(to_string(fullName));
	}
	
	bool MContentManager::MoveElement(uint id, String^ newFullPath)
	{
	    bool res = this->contentManager->MoveElement(id, to_string(newFullPath));
	    if (res)
	        this->OnChanged(nullptr);
	    return res;
	}
	
	bool MContentManager::DeleteElement(uint id)
	{
	    bool res = this->contentManager->DeleteElement(id);
	    if (res)
	        this->OnChanged(nullptr);
	    return res;
	}
	
	void MContentManager::SaveElement(uint id)
	{
	    this->contentManager->SaveElement(id);
	}
#pragma endregion

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

    MContentElement^ MContentManager::GetElement(uint id)
    {
        return this->GetElement(id, true);
    }

    MContentElement^ MContentManager::GetElement(uint id, bool load)
    {
        return this->getMContentElement(this->contentManager->GetElement(id, load, true));
    }

    MContentElement^ MContentManager::GetElement(String^ fullName)
    {
        return this->GetElement(fullName, true);
    }

    MContentElement^ MContentManager::GetElement(String^ fullName, bool load)
    {
        return this->getMContentElement(this->contentManager->GetElement(to_string(fullName), load, true));
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
		MContentElement^ mce = MContentManager::GetMContentElement(element);

		if (mce != nullptr)
			mce->Changed += gcnew MContentElement::ChangedEventHandler(this, &MContentManager::OnElementChanged);
		return mce;
	}


	MContentElement^ MContentManager::GetMContentElement(const ContentElementPtr& element)
	{
		if (!element)
			return nullptr;

		if (!element->IsLoaded)
			return gcnew MContentElement(element->Owner, element->ID);

		MContentElement^ mce = nullptr;
		if (element->Type == ContentElementType::EMesh)
			mce = gcnew MMesh(element->Owner, element->ID);
		else if (element->Type == ContentElementType::EMaterial)
			mce = gcnew MMaterial(element->Owner, element->ID);
		else if (element->Type == ContentElementType::ETexture)
			mce = gcnew MTexture(element->Owner, element->ID);
		/* TODO: add content elements */
		return mce;
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
