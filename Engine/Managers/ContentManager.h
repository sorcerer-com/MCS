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

		bool ImportPackage(const string& filePath);
		bool ExportToPackage(const string& filePath, uint id);

		bool CreatePath(const string& fullPath);
		bool RenamePath(const string& oldFullPath, const string& newFullPath);
		bool ContainPath(const string& fullPath) const;
		bool DeletePath(const string& fullPath);
		vector<string> GetPaths() const;

		ContentElementPtr AddElement(ContentElementType type, const string& name, const string& package, const string& path, uint id = 0);
		bool AddElement(ContentElement* element);
		bool ContainElement(uint id) const;
		bool ContainElement(const string& fullName);
		bool MoveElement(uint id, const string& newFullPath);
		bool DeleteElement(uint id);
		ContentElementPtr GetElement(uint id, bool load, bool waitForLoad = false);
		ContentElementPtr GetElement(const string& fullName, bool load, bool waitForLoad = false);
		vector<ContentElementPtr> GetElements();
		void SaveElement(uint id);
		
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

