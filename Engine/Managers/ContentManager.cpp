// ContentManager.cpp

#include "stdafx.h"
#include "ContentManager.h"

#include "..\Scene.h"
#include "..\Utils\Config.h"
#include "..\Utils\IOUtils.h"
#include "..\Content Elements\ContentElement.h"


namespace Engine {

	ContentManager::ContentManager()
	{
		this->interrupt = false;
		this->worker = thread(&ContentManager::doSerilization, this);

		this->requests.push(make_pair(ELoadDatabase, 0));

		ContentElement* elem = new ContentElement(EMesh, "name", "package", "path");
		this->AddElement(elem);
	}


	ContentManager::~ContentManager()
	{
		this->interrupt = true;
		this->worker.join();
	}


	bool ContentManager::AddElement(ContentElement* element)
	{
		if (!element)
			throw "ArgumentNullException: element";

		if (element->ID == INVALID_ID)
		{
			do {
				element->ID = (uint)Now;
			} while (this->ContainElement(element->ID));
		}

		if (this->ContainElement(element->ID) || this->GetElement(element->Path + element->Name, false) != NULL)
			Scene::Log(EWarning, "ContentManager", "Add content element '" + element->Path + element->Name + "' (" + to_string(element->ID) + ") that already exists");

		this->contentMutex.lock();
		this->content[element->ID] = element;
		this->packageInfos[element->Package].Paths[element->Path].push_back(element->ID);
		this->contentMutex.unlock();

		this->requests.push(make_pair(ESaveDatabase, 0));

		Scene::Log(ELog, "ContentManager", "Add content element '" + element->Name + "'#" +	to_string(element->Version) + " (" + to_string(element->ID) + ")");
		return true;
	}

	bool ContentManager::ContainElement(uint id) const
	{
		return this->content.find(id) != this->content.end();
	}

	ContentElement* ContentManager::GetElement(uint id, bool load)
	{
		if (this->content.find(id) == this->content.end())
		{
			Scene::Log(EWarning, "ContentManager", "Try to get non existent content element (" + to_string(id) + ")");
			return NULL;
		}

		// TODO:
		// load element if it isn't
		if (load)
			//this->LoadElement(id);
			throw "Not implemented";

		return this->content[id];
	}

	ContentElement* ContentManager::GetElement(const string& fullname, bool load)
	{
		string package = ContentElement::GetPackage(fullname);
		string path = ContentElement::GetPath(fullname);
		string name = ContentElement::GetName(fullname);

		if (this->packageInfos.find(package) == this->packageInfos.end())
		{
			Scene::Log(EWarning, "ContentManager", "Try to get non existent content element '" + path + "'");
			return NULL;
		}
		PackageInfo& info = this->packageInfos[package];

		if (info.Paths.find(path) == info.Paths.end())
		{
			Scene::Log(EWarning, "ContentManager", "Try to get non existent content element '" + path + "'");
			return NULL;
		}

		vector<uint>& ids = info.Paths[path];
		for (int i = 0; i < (int)ids.size(); i++)
		{
			ContentElement* elem = this->GetElement(ids[i], false);
			if (elem->Name.compare(name) == 0)
				return this->GetElement(ids[i], load);
		}

		return NULL;
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
					this->saveDatabase();
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
		string filePath = string(CONTENT_FOLDER) + string("\\") + string(CONTENT_DB_FILE);
		Scene::Log(ELog, "ContentManager", "Load database file: " + filePath);

		ifstream ifile(filePath, ios_base::in | ios_base::binary);
		if (!ifile || !ifile.is_open())
		{
			Scene::Log(EError, "ContentManager", "Cannot load database file: " + filePath);
			ifile.close();
			return;
		}

		// Package Infos
		size_t size = 0;
		Read(ifile, size);
		for (int i = 0; i < size; i++)
		{
			string packageName;
			Read(ifile, packageName);
			PackageInfo info;

			string str;
			size_t size2 = 0;
			Read(ifile, size2);
			for (int j = 0; j < size2; j++)
			{
				Read(ifile, str);
				info.Paths[str];
			}

			size_t s1, s2;
			size2 = 0;
			Read(ifile, size2);
			for (int j = 0; j < size2; j++)
			{
				Read(ifile, s1);
				Read(ifile, s2);
				info.FreeSpaces.push_back(make_pair(s1, s2));
			}


			this->packageInfos[packageName] = info;
		}

		// Content Elements
		size = 0;
		Read(ifile, size);
		for (int i = 0; i < size; i++)
		{
			ContentElement* element = new ContentElement(ifile);

			if (element && !ifile.fail())
				this->AddElement(element);
		}

		ifile.close();
	}

	void ContentManager::saveDatabase()
	{
		string filePath = string(CONTENT_FOLDER) + string("\\") + string(CONTENT_DB_FILE);
		Scene::Log(ELog, "ContentManager", "Save database file: " + filePath);

		// Save
		ofstream ofile(filePath, ios_base::out | ios_base::binary);
		if (!ofile || !ofile.is_open())
		{
			Scene::Log(EError, "ContentManager", "Cannot create database file: " + filePath);
			ofile.close();
			return;
		}

		// Package Infos
		Write(ofile, this->packageInfos.size());
		for (auto& pair : this->packageInfos)
		{
			Write(ofile, pair.first);
			PackageInfo& info = pair.second;

			Write(ofile, info.Paths.size());
			for (auto it = info.Paths.begin(); it != info.Paths.end(); it++)
				Write(ofile, (*it).first);

			Write(ofile, info.FreeSpaces.size());
			for (auto it = info.FreeSpaces.begin(); it != info.FreeSpaces.end(); it++)
			{
				Write(ofile, (*it).first);
				Write(ofile, (*it).second);
			}
		}
		
		// Content Elements
		Write(ofile, this->content.size());
		for (auto it = this->content.begin(); it != this->content.end(); it++)
		{
			ContentElement* element = (*it).second;
			element->ContentElement::WriteToFile(ofile);
		}

		ofile.flush();
		ofile.close();
	}

}
