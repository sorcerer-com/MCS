// ContentElement.cpp

#include "stdafx.h"
#include "ContentElement.h"

#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"


namespace MyEngine {

    ContentElement::ContentElement(ContentManager* owner, ContentElementType type, const string& name, const string& package, const string& path)
    {
        if (!owner)
            throw "ArgumentNullException: owner";

        this->Owner = owner;

        this->init();
        this->Type = type;
        this->Name = name;
        this->Package = package;
        this->Path = path;
    }

    ContentElement::ContentElement(ContentManager* owner, istream& file)
    {
        if (!owner)
            throw "ArgumentNullException: owner";

        this->Owner = owner;

        this->init();
        Read(file, this->Version);
        if (this->Version >= 1)
        {
#pragma region ContentElement Read
            Read(file, this->Type);
            Read(file, this->ID);
            Read(file, this->Name);
            Read(file, this->Package);
            Read(file, this->Path);
            Read(file, this->PackageOffset);
            Read(file, this->SavedSize);
#pragma endregion
        }
        this->IsLoaded = false;
    }

    ContentElement::~ContentElement()
    {
    }

	void ContentElement::init()
    {
#pragma region ContentElement Init
        this->Version = CURRENT_VERSION;
        this->Type = ContentElementType::EMesh;
        this->ID = INVALID_ID;
        this->Name = "";
        this->Package = "";
        this->Path = "";
        this->PackageOffset = 0;
        this->SavedSize = 0;
        this->IsLoaded = false;
#pragma endregion
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


    void* ContentElement::get(const string& name)
    {
#pragma region ContentElement Get
#pragma endregion
        return NULL;
    }

	long long ContentElement::Size() const
	{
		long long size = 0;
        size += SizeOf(CURRENT_VERSION);
#pragma region ContentElement Size
        size += SizeOf(this->Type);
        size += SizeOf(this->ID);
        size += SizeOf(this->Name);
        size += SizeOf(this->Package);
        size += SizeOf(this->Path);
        size += SizeOf(this->PackageOffset);
        size += SizeOf(this->SavedSize);
#pragma endregion
		return size;
	}

	void ContentElement::WriteToFile(ostream& file)
    {
        this->SavedSize = this->Size();
        Write(file, CURRENT_VERSION);
#pragma region ContentElement Write
        Write(file, this->Type);
        Write(file, this->ID);
        Write(file, this->Name);
        Write(file, this->Package);
        Write(file, this->Path);
        Write(file, this->PackageOffset);
        Write(file, this->SavedSize);
#pragma endregion
		file.flush();
	}

	ContentElement* ContentElement::Clone() const
	{
		ContentElement* newElem = new ContentElement(*this);
		newElem->ID = INVALID_ID;
		newElem->PackageOffset = 0;
		newElem->SavedSize = 0;
		newElem->IsLoaded = false;
		return newElem;
	}

}
