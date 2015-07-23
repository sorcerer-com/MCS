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
			Read(file, this->AmbientColor);
			Read(file, this->DiffuseColor);
			Read(file, this->SpecularColor);
			Read(file, this->Shininess);
            Read(file, this->Glossiness);
            Read(file, this->IOR);
			Read(file, this->Textures);
		}
		this->IsLoaded = true;
	}

	void Material::init()
	{
		this->AmbientColor = Color4(0.2f, 0.2f, 0.2f, 1.0f);
		this->DiffuseColor = Color4(0.8f, 0.8f, 0.8f, 1.0f);
		this->SpecularColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);
		this->Shininess = 10.0f;
		this->Glossiness = 1.0f;
        this->IOR = 1.5f;
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


	long long Material::Size() const
	{
		long long size = ContentElement::Size();
		size += SizeOf(this->AmbientColor);
		size += SizeOf(this->DiffuseColor);
		size += SizeOf(this->SpecularColor);
		size += SizeOf(this->Shininess);
        size += SizeOf(this->Glossiness);
        size += SizeOf(this->IOR);
		size += SizeOf(this->Textures);
		return size;
	}

	void Material::WriteToFile(ostream& file)
	{
		ContentElement::WriteToFile(file);

		Write(file, this->AmbientColor);
		Write(file, this->DiffuseColor);
		Write(file, this->SpecularColor);
		Write(file, this->Shininess);
        Write(file, this->Glossiness);
        Write(file, this->IOR);
        Write(file, this->Textures);
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