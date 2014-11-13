// ContentElement.cpp

#include "stdafx.h"
#include "ContentElement.h"

#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"


namespace Engine {

	ContentElement::ContentElement(ContentElementType type, const string& name, const string& package, const string& path)
	{
		this->Version = CURRENT_VERSION;
		this->ID = INVALID_ID;
		this->Type = type;
		this->Name = name;
		this->Package = package;
		this->Path = path;
		this->Lock = "";

		this->PackageOffset = 0;
		this->Size = 0;
		this->IsLoaded = false;
	}

	ContentElement::ContentElement(istream& file) : ContentElement()
	{
		Read(file, this->Version);
		if (this->Version >= 1)
		{
			Read(file, this->ID);
			Read(file, this->Type);
			Read(file, this->Name);
			Read(file, this->Package);
			Read(file, this->Path);
			Read(file, this->Lock);
			Read(file, this->PackageOffset);
		}

		this->Size = file.tellg() - this->PackageOffset;
	}

	ContentElement::~ContentElement()
	{
	}


	string ContentElement::GetFullName() const
	{
		return this->Package + "#" + this->Path + "\\" + this->Name;
	}


	void ContentElement::WriteToFile(ostream& file) const
	{
		Write(file, CURRENT_VERSION);
		Write(file, this->ID);
		Write(file, this->Type);
		Write(file, this->Name);
		Write(file, this->Package);
		Write(file, this->Path);
		Write(file, this->Lock);
		Write(file, this->PackageOffset);
		file.flush();
	}

	ContentElement* ContentElement::Clone() const
	{
		ContentElement* newElem = new ContentElement(*this);
		newElem->ID = INVALID_ID;
		newElem->PackageOffset = 0;
		return newElem;
	}


	string ContentElement::GetPackage(const string& fullName)
	{
		return fullName.substr(0, fullName.find_last_of("#") + 1);
	}

	string ContentElement::GetPath(const string& fullName)
	{
		if (fullName.find("\\") != string::npos)
			return fullName.substr(0, fullName.find_last_of("\\") + 1);
		else
			return fullName.substr(0, fullName.find_last_of("#") + 1);
	}

	string ContentElement::GetName(const string& fullName)
	{
		if (fullName.find("\\") != string::npos)
			return fullName.substr(fullName.find_last_of("\\") + 1);
		else
			return fullName.substr(fullName.find_last_of("#") + 1);
	}

}