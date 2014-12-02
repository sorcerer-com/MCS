// ContentElement.h
#pragma once

#include "..\Utils\Header.h"

namespace Engine {

	class ContentManager;

	enum ContentElementType
	{
		EMesh,
		EMaterial,
		ETexture,
		EUIScreen,
		ESkeleton,
		ESound
	};

	class ContentElement
	{
	public:
		uint Version;
		ContentElementType Type;
		uint ID;
		string Name;
		string Package;
		string Path;

		long long PackageOffset;
		bool IsLoaded;

		ContentManager* Owner;

	public:
		ContentElement(ContentManager* owner, ContentElementType type, const string& name, const string& package, const string& path);
		ContentElement(ContentManager* owner, istream& file);

		string GetFullPath() const;
		string GetFullName() const;

		virtual long long Size() const;
		virtual void WriteToFile(ostream& file) const;
		virtual ContentElement* Clone() const;

	public:
		static string GetPackage(const string& fullName);
		static string GetPath(const string& fullName);
		static string GetName(const string& fullName);
	};

}

