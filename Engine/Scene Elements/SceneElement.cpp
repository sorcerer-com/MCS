// SceneElement.cpp

#include "stdafx.h"
#include "SceneElement.h"

#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"

#include "..\Managers\SceneManager.h"
#include "..\Engine.h"
#include "..\Managers\ContentManager.h"


namespace MyEngine {

	SceneElement::SceneElement(SceneManager* owner, SceneElementType type, const string& name, uint contentID)
	{
		if (!owner)
			throw "ArgumentNullException: owner";

		this->Version = CURRENT_VERSION;
		this->Type = type;
		this->ID = INVALID_ID;
		this->Name = name;
		this->ContentID = contentID;
		this->MaterialID = INVALID_ID;
		this->Visible = true;
		this->Position = Vector3();
		this->Rotation = Quaternion();
		this->Scale = Vector3(1.0f, 1.0f, 1.0f);

		this->Owner = owner;
	}

	SceneElement::SceneElement(SceneManager* owner, istream& file)
	{
		if (!owner)
			throw "ArgumentNullException: owner";

		Read(file, this->Version);
		if (this->Version >= 1)
		{
			Read(file, this->Type);
			Read(file, this->ID);
			Read(file, this->Name);
			Read(file, this->ContentID);
			Read(file, this->MaterialID);
			Read(file, this->Visible);
			Read(file, this->Position);
			Read(file, this->Rotation);
			Read(file, this->Scale);
		}

		this->Owner = owner;
	}


	ContentElementPtr SceneElement::GetContent() const
	{
		return this->Owner->Owner->ContentManager->GetElement(this->ContentID, true, true);
	}

	ContentElementPtr SceneElement::GetMaterial() const
	{
		return this->Owner->Owner->ContentManager->GetElement(this->MaterialID, true, true);
	}


	void SceneElement::WriteToFile(ostream& file) const
	{
		Write(file, CURRENT_VERSION);
		Write(file, this->Type);
		Write(file, this->ID);
		Write(file, this->Name);
		Write(file, this->ContentID);
		Write(file, this->MaterialID);
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