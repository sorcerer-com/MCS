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

		uint Version;                       //* default[CURRENT_VERSION] nosave readonly
		ContentElementType Type;            //* default[ContentElementType::EMesh] group["Base"] readonly
		uint ID;                            //* default[INVALID_ID] group["Base"] readonly
		string Name;                        //* group["Base"] readonly
		string Package;                     //* readonly
		string Path;                        //* readonly

		long long PackageOffset;            //* noproperty
		long long SavedSize;                //* noproperty
		bool IsLoaded;                      //* nosave group["Base"] readonly

	public:
		ContentElement(ContentManager* owner, ContentElementType type, const string& name, const string& package, const string& path);
		ContentElement(ContentManager* owner, istream& file);
        virtual ~ContentElement();

		string GetFullPath() const;
		string GetFullName() const;

        template <typename T>
        T& Get(const string& name) { return *((T*)this->get(name)); }
		virtual long long Size() const;
		virtual void WriteToFile(ostream& file);
		virtual ContentElement* Clone() const;

	protected:
        void init();
        virtual void* get(const string& name);
	};

}

