// SceneElement.cpp

#include "stdafx.h"
#include "SceneElement.h"

#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"

#include "..\Engine.h"
#include "..\Managers\SceneManager.h"
#include "..\Managers\ContentManager.h"


namespace MyEngine {

	SceneElement::SceneElement(SceneManager* owner, SceneElementType type, const string& name, uint contentID)
	{
		if (!owner)
			throw "ArgumentNullException: owner";

		this->Owner = owner;

		this->init();
		this->Type = type;
		this->Name = name;
		this->ContentID = contentID;
	}

	SceneElement::SceneElement(SceneManager* owner, istream& file)
	{
		if (!owner)
			throw "ArgumentNullException: owner";

		this->Owner = owner;

		this->init();
		Read(file, this->Version);
		if (this->Version >= 1)
		{
			Read(file, this->Type);
			Read(file, this->ID);
			Read(file, this->Name);
			Read(file, this->Layer);
			Read(file, this->ContentID);
            Read(file, this->MaterialID);
            Read(file, this->Textures);
			Read(file, this->Visible);
			Read(file, this->Position);
			Read(file, this->Rotation);
			Read(file, this->Scale);
		}
	}

    SceneElement::~SceneElement()
    {
    }

	void SceneElement::init()
	{
		this->Version = CURRENT_VERSION;
		this->Type = SceneElementType::ECamera;
		this->ID = INVALID_ID;
		this->Name = "";
		this->Layer = DEFAULT_LAYER_NAME;
		this->ContentID = INVALID_ID;
		this->MaterialID = INVALID_ID;
		this->Visible = true;
		this->Position = Vector3();
		this->Rotation = Quaternion();
		this->Scale = Vector3(1.0, 1.0, 1.0);
	}


	ContentElementPtr SceneElement::GetContent() const
	{
		if (this->Owner->Owner->ContentManager->ContainElement(this->ContentID))
			return this->Owner->Owner->ContentManager->GetElement(this->ContentID, true, true);
		return ContentElementPtr();
	}

	ContentElementPtr SceneElement::GetMaterial() const
	{
		if (this->Owner->Owner->ContentManager->ContainElement(this->MaterialID))
			return this->Owner->Owner->ContentManager->GetElement(this->MaterialID, true, true);
		return ContentElementPtr();
    }

    ContentElementPtr SceneElement::GetDiffuseMap() const
    {
        if (this->Owner->Owner->ContentManager->ContainElement(this->Textures.DiffuseMapID))
            return this->Owner->Owner->ContentManager->GetElement(this->Textures.DiffuseMapID, true, true);
        return ContentElementPtr();
    }

    ContentElementPtr SceneElement::GetNormalMap() const
    {
        if (this->Owner->Owner->ContentManager->ContainElement(this->Textures.NormalMapID))
            return this->Owner->Owner->ContentManager->GetElement(this->Textures.NormalMapID, true, true);
        return ContentElementPtr();
    }


	void SceneElement::WriteToFile(ostream& file) const
	{
		Write(file, CURRENT_VERSION);
		Write(file, this->Type);
		Write(file, this->ID);
		Write(file, this->Name);
		Write(file, this->Layer);
		Write(file, this->ContentID);
        Write(file, this->MaterialID);
        Write(file, this->Textures);
		Write(file, this->Visible);
		Write(file, this->Position);
		Write(file, this->Rotation);
		Write(file, this->Scale);
		file.flush();
	}

	SceneElement* SceneElement::Clone() const
	{
		SceneElement* elem = new SceneElement(*this);
		elem->ID = INVALID_ID;
		return elem;
	}

}