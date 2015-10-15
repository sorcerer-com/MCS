// Mesh.cpp

#include "stdafx.h"
#include "Material.h"

#include "..\Engine.h"
#include "..\Utils\Config.h"
#include "..\Utils\Utils.h"
#include "..\Utils\IOUtils.h"

#include "..\Managers\ContentManager.h"


namespace MyEngine {

	Material::Material(ContentManager* owner, const string& name, const string& package, const string& path) :
		ContentElement(owner, EMaterial, name, package, path)
	{
		this->init();
		this->IsLoaded = true;
	}

	Material::Material(ContentManager* owner, istream& file) :
		ContentElement(owner, file)
	{
		this->init();
		if (this->Version >= 1)
        {
#pragma region Material Read
            Read(file, this->DiffuseColor);
            Read(file, this->SpecularColor);
            Read(file, this->InnerColor);
            Read(file, this->Shininess);
            Read(file, this->Glossiness);
            Read(file, this->IOR);
            Read(file, this->Absorption);
            Read(file, this->Textures);
#pragma endregion
		}
		this->IsLoaded = true;
	}

	void Material::init()
    {
#pragma region Material Init
        this->DiffuseColor = Color4(0.8f, 0.8f, 0.8f, 1.0f);
        this->SpecularColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);
        this->InnerColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);
        this->Shininess = 10.0f;
        this->Glossiness = 1.0f;
        this->IOR = 1.5f;
        this->Absorption = 0.1f;
#pragma endregion
        this->Textures.DiffuseMapID = INVALID_ID;
        this->Textures.NormalMapID = INVALID_ID;
	}


	ContentElementPtr Material::GetDiffuseMap() const
	{
        if (this->Owner->ContainsElement(this->Textures.DiffuseMapID))
            return this->Owner->GetElement(this->Textures.DiffuseMapID, true, true);
		return ContentElementPtr();
	}

	ContentElementPtr Material::GetNormalMap() const
	{
        if (this->Owner->ContainsElement(this->Textures.NormalMapID))
            return this->Owner->GetElement(this->Textures.NormalMapID, true, true);
		return ContentElementPtr();
	}


    void* Material::get(const string& name)
    {
#pragma region Material Get
        if (name == "DiffuseColor")
            return &this->DiffuseColor;
        else if (name == "SpecularColor")
            return &this->SpecularColor;
        else if (name == "InnerColor")
            return &this->InnerColor;
        else if (name == "Shininess")
            return &this->Shininess;
        else if (name == "Glossiness")
            return &this->Glossiness;
        else if (name == "IOR")
            return &this->IOR;
        else if (name == "Absorption")
            return &this->Absorption;
#pragma endregion
        return ContentElement::get(name);
    }

	long long Material::Size() const
	{
        long long size = ContentElement::Size();
#pragma region Material Size
        size += SizeOf(this->DiffuseColor);
        size += SizeOf(this->SpecularColor);
        size += SizeOf(this->InnerColor);
        size += SizeOf(this->Shininess);
        size += SizeOf(this->Glossiness);
        size += SizeOf(this->IOR);
        size += SizeOf(this->Absorption);
        size += SizeOf(this->Textures);
#pragma endregion
		return size;
	}

	void Material::WriteToFile(ostream& file)
	{
		ContentElement::WriteToFile(file);

#pragma region Material Write
		Write(file, this->DiffuseColor);
		Write(file, this->SpecularColor);
		Write(file, this->InnerColor);
		Write(file, this->Shininess);
		Write(file, this->Glossiness);
		Write(file, this->IOR);
		Write(file, this->Absorption);
		Write(file, this->Textures);
#pragma endregion
		file.flush();
	}

	ContentElement* Material::Clone() const
	{
		Material* newElem = new Material(*this);
		newElem->ID = INVALID_ID;
		newElem->PackageOffset = 0;
		newElem->SavedSize = 0;
		return newElem;
	}

}
