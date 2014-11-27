// ContentElement.cpp

#include "stdafx.h"
#include "ContentElement.h"

#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"


namespace Engine {

	ContentElement::ContentElement(ContentManager* owner, ContentElementType type, const string& name, const string& package, const string& path)
	{
		this->Version = CURRENT_VERSION;
		this->Type = type;
		this->ID = INVALID_ID;
		this->Name = name;
		this->Package = package;
		this->Path = path;

		this->PackageOffset = 0;
		this->IsLoaded = false;

		this->Owner = owner;
	}

	ContentElement::ContentElement(ContentManager* owner, istream& file)
	{
		Read(file, this->Version);
		if (this->Version >= 1)
		{
			Read(file, this->Type);
			Read(file, this->ID);
			Read(file, this->Name);
			Read(file, this->Package);
			Read(file, this->Path);
			Read(file, this->PackageOffset);
		}
		this->IsLoaded = false;

		this->Owner = owner;
	}

	ContentElement::~ContentElement()
	{
	}


	string ContentElement::GetFullPath() const
	{
		string fullPath = this->Package + "#";
		if (this->Path != "")
			fullPath += this->Path + "\\";
		return fullPath;
	}

	string ContentElement::GetFullName() const
	{
		return this->GetFullPath() + this->Name;
	}


	long long ContentElement::Size() const
	{
		long long size = 0;
		size += SizeOf(CURRENT_VERSION);
		size += SizeOf(this->Type);
		size += SizeOf(this->ID);
		size += SizeOf(this->Name);
		size += SizeOf(this->Package);
		size += SizeOf(this->Path);
		size += SizeOf(this->PackageOffset);
		return size;
	}

	void ContentElement::WriteToFile(ostream& file) const
	{
		Write(file, CURRENT_VERSION);
		Write(file, this->Type);
		Write(file, this->ID);
		Write(file, this->Name);
		Write(file, this->Package);
		Write(file, this->Path);
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
		return fullName.substr(0, fullName.find_last_of("#"));
	}

	string ContentElement::GetPath(const string& fullName)
	{
		size_t start = fullName.find_last_of("#") + 1;
		size_t end = fullName.find_last_of("\\");
		if (end != string::npos)
			return fullName.substr(start, end - start);
		else
			return "";
	}

	string ContentElement::GetName(const string& fullName)
	{
		size_t slash = fullName.find_last_of("\\");
		size_t hash = fullName.find_last_of("#");
		if (slash != string::npos)
		{
			if (slash > hash)
				return fullName.substr(slash + 1);
			else
				return "";
		}
		else
			return fullName.substr(hash + 1);
	}

}