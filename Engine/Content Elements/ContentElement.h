// ContentElement.h
#pragma once

#include "..\Utils\Header.h"

namespace Engine {

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
		uint ID;
		ContentElementType Type;
		string Name;
		string Package;
		string Path;
		string Lock;

		streampos PackageOffset;
		streampos Size;
		bool IsLoaded;

	private:
		ContentElement() = default;

	public:
		ContentElement(ContentElementType type, const string& name, const string& package, const string& path);
		ContentElement(istream& file);
		virtual ~ContentElement();

		string GetFullName() const;

		virtual void WriteToFile(ostream& file) const;
		virtual ContentElement* Clone() const;

	public:
		static string GetPackage(const string& fullName);
		static string GetPath(const string& fullName);
		static string GetName(const string& fullName);
	};

}

