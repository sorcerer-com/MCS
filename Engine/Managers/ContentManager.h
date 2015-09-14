// ContentManager.h
#pragma once

#include "BaseManager.h"
#include "..\Utils\Header.h"


namespace MyEngine {

	class ContentElement;
	enum ContentElementType;

	using ContentElementPtr = shared_ptr < ContentElement >;

	class ContentManager : public BaseManager
	{
	private:
		struct PackageInfo
		{
			map < string, set < uint > > Paths;
			vector < pair< long long, long long > > FreeSpaces;
		};

		enum RequestType
		{
			ELoadDatabase,
			ESaveDatabase,
			ELoadElement,
			ESaveElement
		};

	public:
		using RequestQueueType = deque < pair<RequestType, uint> >; // type / id

		using PackageInfoMapType = map < string, PackageInfo > ; // package name / package info
		using ContentMapType = map < uint, ContentElementPtr >; // id / content element

	private:
		RequestQueueType requests;

		PackageInfoMapType packageInfos;
		ContentMapType content;
		// TODO: instances

	public:
		ContentManager(Engine* owner);
        ~ContentManager();

        bool ImportPackage(const string& filePath);	//* wrap
        bool ExportToPackage(const string& filePath, uint id);	//* wrap const endgroup

        bool CreatePath(const string& fullPath);	//* wrap
        bool RenamePath(const string& oldFullPath, const string& newFullPath);	//* wrap
        bool ContainsPath(const string& fullPath) const;	//* wrap
        bool DeletePath(const string& fullPath);	//* wrap endgroup
        vector<string> GetPaths() const;

        ContentElementPtr AddElement(ContentElementType type, const string& name, const string& package, const string& path, uint id = 0);	//* wrap
        bool AddElement(ContentElement* element);	//* wrap
        bool ContainsElement(uint id) const;		//* wrap
        bool ContainsElement(const string& fullName);	//* wrap const
        bool MoveElement(uint id, const string& newFullPath);	//* wrap
        bool DeleteElement(uint id);				//* wrap
        ContentElementPtr GetElement(uint id, bool load, bool waitForLoad = false);	//* wrap const
        ContentElementPtr GetElement(const string& fullName, bool load, bool waitForLoad = false);	//* wrap const
        vector<ContentElementPtr> GetElements() const;
        void SaveElement(uint id);					//* wrap const
		
	private:
		void doSerilization();
		void addRequest(RequestType type, uint id = 0);

		void loadDatabase();
		void saveDatabase();
		bool loadElement(uint id);
		ContentElement* loadElement(istream& ifile, ContentElementType type);
		bool saveElement(uint id, bool backup = true);
		bool eraseElement(uint id, bool backup = true);
		void beckupElement(const ContentElementPtr& element, bool erase);
		bool unLoadElement(uint id);

	public:
		static string GetPackage(const string& fullName);
		static string GetPath(const string& fullName);
		static string GetName(const string& fullName);

	};

}

