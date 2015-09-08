// ContentElement.h
#pragma once

#include "..\Utils\Header.h"

namespace MyEngine {

	class ContentManager;

	enum ContentElementType
	{
		EMesh,
		EMaterial,
		ETexture
	};

	class ContentElement
	{
	public:
		ContentManager* Owner;              //* noinit

		uint Version;                       //* default[CURRENT_VERSION] nosave
		ContentElementType Type;            //* default[ContentElementType::EMesh]
		uint ID;                            //* default[INVALID_ID]
		string Name;                        //*
		string Package;                     //*
		string Path;                        //*

		long long PackageOffset;            //*
		long long SavedSize;                //*
		bool IsLoaded;                      //* nosave

	public:
		ContentElement(ContentManager* owner, ContentElementType type, const string& name, const string& package, const string& path);
		ContentElement(ContentManager* owner, istream& file);
        virtual ~ContentElement();

		string GetFullPath() const;
		string GetFullName() const;

		virtual long long Size() const;
		virtual void WriteToFile(ostream& file);
		virtual ContentElement* Clone() const;

	private:
		void init();
	};

}

