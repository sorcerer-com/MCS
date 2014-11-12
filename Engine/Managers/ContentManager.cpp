// ContentManager.cpp

#include "stdafx.h"
#include "ContentManager.h"

#include "..\Scene.h"


namespace Engine {

	ContentManager::ContentManager()
	{
		this->worker = thread(&ContentManager::doSerilization, this);
		this->interrupt = false;

		this->requests.push(make_pair(ELoadDatabase, 0));
	}


	ContentManager::~ContentManager()
	{
		this->interrupt = true;
		this->worker.join();
	}


	void ContentManager::doSerilization()
	{
		while (!this->interrupt)
		{
			while (!this->requests.empty())
			{
				this->requestsMutex.lock();
				auto request = this->requests.front();
				this->requests.pop();
				this->requestsMutex.unlock();

				switch (request.first)
				{
				case ELoadDatabase:
					this->loadDatabase();
					break;
				case ESaveDatabase:
					break;
				default:
					Scene::Log(EWarning, "ContentManager", "Invalid request");
					break;
				}

				this_thread::sleep_for(chrono::milliseconds(1));
			}

			this_thread::sleep_for(chrono::milliseconds(100));
		}
	}

	void ContentManager::loadDatabase()
	{

	}

	void ContentManager::saveDatabase()
	{

	}

}
