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
		enum RequestType
		{
			ELoadDatabase,
			ESaveDatabase
		};

	public:
		using PathMapType = map < string, vector<uint> >; // path / vector with ids
		using ContentMapType = map < uint, ContentElement* > ; // id / content element
		using RequestQueueType = queue < pair<RequestType, uint> > ; // type / id

	private:
		thread worker;
		atomic_bool interrupt;
		RequestQueueType requests;
		mutex requestsMutex;


		PathMapType paths;
		ContentMapType content;

	public:
		ContentManager();
		~ContentManager();

	private:
		void doSerilization();
		void loadDatabase();
		void saveDatabase();

	};

}

