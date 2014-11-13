// ContentManager.h
#pragma once

#include <thread>
#include <atomic>
#include <mutex>

#include "..\Utils\Header.h"


namespace Engine {

	class Scene;
	class ContentElement;

	class ContentManager
	{
	private:
		struct PackageInfo
		{
			map < string, vector < uint > > Paths;
			vector < pair<size_t, size_t > > FreeSpaces;
		};

		enum RequestType
		{
			ELoadDatabase,
			ESaveDatabase
		};

	public:
		using RequestQueueType = queue < pair<RequestType, uint> >; // type / id

		using PackageInfoMapType = map < string, PackageInfo > ; // package name / package info
		using ContentMapType = map < uint, ContentElement* > ; // id / content element
		
	private:
		thread worker;
		atomic_bool interrupt;
		RequestQueueType requests;
		mutex requestsMutex;


		PackageInfoMapType packageInfos;
		ContentMapType content;
		mutex contentMutex;

	public:
		ContentManager();
		~ContentManager();

		bool AddElement(ContentElement* element);
		bool ContainElement(uint id) const;
		ContentElement* GetElement(uint id, bool load);
		ContentElement* GetElement(const string& fullName, bool load);

	private:
		void doSerilization();
		void loadDatabase();
		void saveDatabase();

	};

}

