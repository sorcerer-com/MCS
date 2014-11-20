// ContentManager.cpp

#include "stdafx.h"
#include "ContentManager.h"

#include "..\Scene.h"
#include "..\Utils\Config.h"
#include "..\Utils\Thread.h"
#include "..\Utils\IOUtils.h"
#include "..\Content Elements\ContentElement.h"


namespace Engine {

	/* C O N T E N T   M A N A G E R */
	ContentManager::ContentManager()
	{
		this->thread = make_shared<Thread>();
		this->thread->worker(&ContentManager::doSerilization, this);
		this->thread->defMutex("requestsMutex");
		this->thread->defMutex("contentMutex");

		this->addRequest(ELoadDatabase, true);
	}

	ContentManager::~ContentManager()
	{
		this->thread->join();
	}


	/* P A C K A G E S */
	// TODO: comments
	bool ContentManager::ImportPackage(const string& filePath)
	{
		// Load
		ifstream ifile(filePath, ios_base::in | ios_base::binary);
		if (!ifile || !ifile.is_open())
		{
			Scene::Log(EError, "ContentManager", "Cannot import package file: " + filePath);
			ifile.close();
			return false;
		}

		while (!ifile.eof())
		{
			uint version = 0u;
			while (version == 0u)
			{
				Read(ifile, version);
				if (ifile.eof())
					break;
			}
			if (version == 0u)
				break;
			ContentElementType type = EMesh;
			Read(ifile, type);
			const streamoff offset = -(streamoff)(sizeof(version) + sizeof(type));
			ifile.seekg(offset, ios_base::cur);

			ContentElement* elem = this->loadElement(ifile, type);

			if (elem && !ifile.fail())
			{
				elem->IsLoaded = true;
				this->thread->mutex("contentMutex").lock();
				this->content[elem->ID] = elem;
				this->thread->mutex("contentMutex").unlock();
			}
			else
			{
				Scene::Log(EError, "ContentManager", "Package file is corrupted: " + filePath);
				ifile.close();
				return false;
			}
		}

		ifile.close();
		Scene::Log(ELog, "ContentManager", "Import package file: " + filePath);
		return true;
	}

	bool ContentManager::ExportToPackage(const string& filePath, uint id)
	{
		lock_guard<mutex> lck(this->thread->mutex("contentMutex"));

		ContentElement* element = this->GetElement(id, false);
		if (!element)
		{
			Scene::Log(EError, "ContentManager", "Cannot export non existent content element (" + to_string(id) + ")");
			return false;
		}

		// Save
		ofstream ofile(filePath, ios_base::app | ios_base::binary);
		if (!ofile || !ofile.is_open())
		{
			Scene::Log(EError, "ContentManager", "Cannot open package file: " + filePath);
			ofile.close();
			return false;
		}

		element->PackageOffset = ofile.tellp();
		element->WriteToFile(ofile);
		ofile.close();

		return true;
	}


	/* P A T H S */
	bool ContentManager::CreatePath(const string& fullPath)
	{
		bool res = !this->ContainPath(fullPath);

		if (res)
		{
			string package = ContentElement::GetPackage(fullPath);
			string path = ContentElement::GetPath(fullPath);
			this->thread->mutex("contentMutex").lock();
			this->packageInfos[package].Paths[path];
			this->thread->mutex("contentMutex").unlock();

			this->addRequest(ESaveDatabase);

			Scene::Log(ELog, "ContentManager", "Create path '" + fullPath + "'");
		}
		else
			Scene::Log(EError, "ContentManager", "Try to create already exists path '" + fullPath + "'");

		return res;
	}

	bool ContentManager::RenamePath(const string& oldFullPath, const string& newFullPath)
	{
		bool res = !this->ContainPath(newFullPath);
		
		if (res == true)
		{
			lock_guard<mutex> lck(this->thread->mutex("contentMutex"));

			string oldPackage = ContentElement::GetPackage(oldFullPath);
			string oldPath = ContentElement::GetPath(oldFullPath);
			PackageInfo& oldInfo = this->packageInfos[oldPackage];

			string newPackage = ContentElement::GetPackage(newFullPath);
			string newPath = ContentElement::GetPath(newFullPath);
			PackageInfo& newInfo = this->packageInfos[newPackage];
			
			vector<string> forDelete;
			for (auto it = oldInfo.Paths.rbegin(); it != oldInfo.Paths.rend(); it++)
			{
				string path = (*it).first;
				if (path.find(oldPath) == 0)
				{
					string npath = path.substr(oldPath.size());
					npath = newPath + npath;

					newInfo.Paths[npath].insert(newInfo.Paths[npath].end(), (*it).second.begin(), (*it).second.end());
					forDelete.push_back(path);

					// change elements paths
					vector<uint>& ids = newInfo.Paths[npath];
					for (auto it2 = ids.begin(); it2 != ids.end(); it2++)
					{
						ContentElement* element = this->GetElement(*it2, true);
						this->addRequest(EEraseElement, *it2, true);
						element->Package = newPackage;
						element->Path = npath;
						this->addRequest(ESaveElement, *it2);
						this->addRequest(ESaveDatabase);
					}

					Scene::Log(ELog, "ContentManager", "Rename path '" + oldPackage + "#" + path + "' to '" + newPackage + "#" + npath + "'");
				}
			}
			for (int i = 0; i < (int)forDelete.size(); i++)
				oldInfo.Paths.erase(forDelete[i]);

			// delete package if it's renamed
			if (oldInfo.Paths.size() == 0)
			{
				this->packageInfos.erase(oldPackage);

				string packFile = CONTENT_FOLDER;
				packFile += "\\" + oldPackage + ".mpk";
				remove(packFile.c_str());
			}
			
			this->addRequest(ESaveDatabase);
		}
		else
			Scene::Log(EError, "ContentManager", "Try to rename non existent path '" + oldFullPath + "'");

		return res;
	}

	bool ContentManager::ContainPath(const string& fullPath) const
	{
		string package = ContentElement::GetPackage(fullPath);
		string path = ContentElement::GetPath(fullPath);
		
		if (this->packageInfos.find(package) == this->packageInfos.end())
			return false;
		const PackageInfo& info = this->packageInfos.at(package);
		if (path != "" && info.Paths.find(path) == info.Paths.end())
			return false;
		return true;
	}

	bool ContentManager::DeletePath(const string& fullPath)
	{
		if (!this->ContainPath(fullPath))
		{
			Scene::Log(EError, "ContentManager", "Try to delete non existent path '" + fullPath + "'");
			return false;
		}

		string package = ContentElement::GetPackage(fullPath);
		string path = ContentElement::GetPath(fullPath);
		PackageInfo& info = this->packageInfos[package];

		vector<string> forDelete;
		for (auto it = info.Paths.rbegin(); it != info.Paths.rend(); it++)
		{
			string currPath = (*it).first;
			if (currPath.find(path) == 0)
			{
				vector<uint>& ids = info.Paths[currPath];
				for (int i = 0; i < (int)ids.size(); i++)
					this->DeleteElement(ids[i]);

				forDelete.push_back(currPath);
			}
		}
		this->thread->mutex("contentMutex").lock();
		for (int i = 0; i < (int)forDelete.size(); i++)
			info.Paths.erase(forDelete[i]);
		this->thread->mutex("contentMutex").unlock();

		// delete package if it's deleted
		if (info.Paths.size() == 0)
		{
			this->thread->mutex("contentMutex").lock();
			this->packageInfos.erase(package);
			this->thread->mutex("contentMutex").unlock();

			string packFile = CONTENT_FOLDER;
			packFile += "\\" + package + ".mpk";
			remove(packFile.c_str());
		}

		Scene::Log(ELog, "ContentManager", "Delete path '" + fullPath + "'");
		return true;
	}


	/* E L E M E N T S */
	ContentElement* ContentManager::AddElement(ContentElementType type, const string& name, const string& package, const string& path, uint id /* = 0 */)
	{
		ContentElement *element = NULL;
		/* TODO: add content elements
		if (type == EMesh)
			element = new Mesh(this, name, path);
		else if (type == EMaterial)
			element = new Material(this, name, path);
		else if (type == ETexture)
			element = new Texture(this, name, path);
		else if (type == EUIScreen)
			element = new UIScreen(this, name, path);
		else if (type == ESkeleton)
			element = new Skeleton(this, name, path);
		else if (type == ESound)
			element = new Sound(this, name, path);*/

		if (element != NULL)
		{
			element->ID = id;
			if (!this->AddElement(element))
				return NULL;
		}
		return element;
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

		if (this->ContainElement(element->ID) || this->GetElement(element->GetFullName(), false) != NULL)
			Scene::Log(EWarning, "ContentManager", "Add content element '" + element->GetFullName() + "' (" + to_string(element->ID) + ") that already exists");

		this->thread->mutex("contentMutex").lock();
		this->content[element->ID] = element;
		this->packageInfos[element->Package].Paths[element->Path].push_back(element->ID);
		this->thread->mutex("contentMutex").unlock();

		this->addRequest(ESaveElement, element->ID);
		this->addRequest(ESaveDatabase);

		Scene::Log(ELog, "ContentManager", "Add content element '" + element->Name + "'#" +	to_string(element->Version) + " (" + to_string(element->ID) + ")");
		return true;
	}

	bool ContentManager::MoveElement(uint id, const string& newFullPath)
	{
		if (!this->ContainElement(id))
		{
			Scene::Log(EError, "ContentManager", "Try to move non existent content element (" + to_string(id) + ")");
			return false;
		}
		if (!this->ContainPath(newFullPath))
		{
			Scene::Log(EError, "ContentManager", "Try to move element to non existent path '" + newFullPath + "'");
			return false;
		}

		ContentElement* element = this->GetElement(id, false);
		if (this->GetElement(newFullPath + "\\" + element->Name, false) != NULL)
		{
			Scene::Log(EError, "ContentManager", "Try to move content element '" + element->Name + "' (" + to_string(element->ID) +
				") to path '" + newFullPath + "', but there is already element with the same name");
			return false;
		}

		this->thread->mutex("contentMutex").lock();
		vector<uint>& ids = this->packageInfos[element->Package].Paths[element->Path];
		auto it = find(ids.begin(), ids.end(), id);
		ids.erase(it);
		element->Package = ContentElement::GetPackage(newFullPath);
		element->Path = ContentElement::GetPath(newFullPath);
		this->packageInfos[element->Package].Paths[element->Path].push_back(element->ID);
		this->thread->mutex("contentMutex").unlock();

		Scene::Log(ELog, "ContentManager", "Move content element '" + element->Name + "' (" + to_string(id) + ") to '" + newFullPath + "'");
		return true;
	}

	bool ContentManager::ContainElement(uint id) const
	{
		return this->content.find(id) != this->content.end();
	}

	bool ContentManager::DeleteElement(uint id)
	{
		lock_guard<mutex> lck(this->thread->mutex("contentMutex"));

		if (!this->ContainElement(id))
		{
			Scene::Log(EError, "ContentManager", "Try to delete non existent content element (" + to_string(id) + ")");
			return false;
		}

		ContentElement* element = this->GetElement(id, false);
		vector<uint>& ids = this->packageInfos[element->Package].Paths[element->Path];
		auto it = find(ids.begin(), ids.end(), id);
		ids.erase(it);
		this->addRequest(EEraseElement, id, true);
		this->content.erase(id);

		this->addRequest(ESaveDatabase);

		Scene::Log(ELog, "ContentManager", "Delete content element (" + to_string(id) + ")");
		return true;
	}

	ContentElement* ContentManager::GetElement(uint id, bool load) // TODO: handle<ContentElement> or shared_ptr<ContentElement> and in doSerialization check if count == 1 then unload
	{
		if (this->content.find(id) == this->content.end())
		{
			Scene::Log(EWarning, "ContentManager", "Try to get non existent content element (" + to_string(id) + ")");
			return NULL;
		}

		// load element if it isn't
		if (load)
			this->addRequest(ELoadElement, id);

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
	
	void ContentManager::SaveElement(uint id)
	{
		this->addRequest(ESaveElement, id);
		this->addRequest(ESaveDatabase);
	}


	/* S E R I L I Z A T I O N */
	void ContentManager::doSerilization()
	{
		while (!this->thread->interrupted())
		{
			while (!this->requests.empty())
			{
				this->thread->mutex("requestsMutex").lock();
				auto request = this->requests.front();
				this->requests.pop_front();
				this->thread->mutex("requestsMutex").unlock();

				switch (request.first)
				{
				case ELoadDatabase:
					this->loadDatabase();
					break;
				case ESaveDatabase:
					this->saveDatabase();
					break;
				case ELoadElement:
					this->loadElement(request.second);
					break;
				case ESaveElement:
					this->saveElement(request.second);
					break;
				case EEraseElement:
					this->eraseElement(request.second);
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

	void ContentManager::addRequest(RequestType type, bool wait)
	{
		this->addRequest(type, 0, wait);
	}

	void ContentManager::addRequest(RequestType type, uint id /* = 0 */, bool wait /* = false */)
	{
		unique_lock<mutex> lck(this->thread->mutex("requestsMutex"));

		auto pair = make_pair(type, id);
		auto it = find(this->requests.begin(), this->requests.end(), pair);

		bool skip = false;
		if (it != this->requests.end() || (type == ESaveDatabase && this->requests.size() > 0 && this->requests.back() == pair))
			skip = true;

		if (!skip)
			this->requests.push_back(pair);
		if (wait)
		{
			while (find(this->requests.begin(), this->requests.end(), pair) != this->requests.end())
			{
				bool isLock = !this->thread->mutex("contentMutex").try_lock();
				this->thread->mutex("contentMutex").unlock();
				
				lck.unlock();
				this_thread::sleep_for(chrono::milliseconds(10));
				lck.lock();

				if (isLock)
					this->thread->mutex("contentMutex").lock();
			}
		}
	}


	void ContentManager::loadDatabase()
	{
		lock_guard<mutex> lck(this->thread->mutex("contentMutex"));

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
		long long size = 0;
		Read(ifile, size);
		for (int i = 0; i < size; i++)
		{
			string packageName;
			Read(ifile, packageName);
			PackageInfo info;

			string str;
			long long size2 = 0;
			Read(ifile, size2);
			for (int j = 0; j < size2; j++)
			{
				Read(ifile, str);
				info.Paths[str];
			}

			long long s1, s2;
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
			ContentElement* element = new ContentElement(this, ifile);

			if (element && !ifile.fail())
			{
				this->content[element->ID] = element;
				this->packageInfos[element->Package].Paths[element->Path].push_back(element->ID);
			}
		}

		ifile.close();
	}

	void ContentManager::saveDatabase()
	{
		lock_guard<mutex> lck(thread->mutex("this->contentMutex"));

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

		ofile.close();
	}

	bool ContentManager::loadElement(uint id)
	{
		ContentElement* element = this->GetElement(id, false);
		if (!element)
		{
			Scene::Log(EError, "ContentManager", "Cannot load non existent content element (" + to_string(id) + ")");
			return false;
		}

		if (element->IsLoaded)
			return true;

		// Load
		string filePath = string(CONTENT_FOLDER) + string("\\") + element->Package + ".mpk";
		ifstream ifile(filePath, ios_base::in | ios_base::binary);
		if (!ifile || !ifile.is_open())
		{
			Scene::Log(EError, "ContentManager", "Cannot load package file: " + filePath);
			ifile.close();
			return false;
		}

		ifile.seekg(element->PackageOffset);

		ContentElement* elem = this->loadElement(ifile, element->Type);

		if (elem && !ifile.fail())
		{
			delete element;
			elem->IsLoaded = true;
			this->thread->mutex("contentMutex").lock();
			this->content[elem->ID] = elem;
			this->thread->mutex("contentMutex").unlock();
		}
		else
		{
			Scene::Log(EError, "ContentManager", "Cannot load content element '" + element->Name + "'#" +
				to_string(element->Version) + " (" + to_string(element->ID) + ")");
			ifile.close();
			return false;
		}

		ifile.close();
		Scene::Log(ELog, "ContentManager", "Load content element '" + elem->Name + "'#" +
			to_string(elem->Version) + " (" + to_string(elem->ID) + ")");
		return true;
	}

	ContentElement* ContentManager::loadElement(istream& ifile, ContentElementType type)
	{
		ContentElement* element = NULL;
		/*if (type == EMesh)
		element = new Mesh(this, ifile);
		else if (type == EMaterial)
		element = new Material(this, ifile);
		else if (type == ETexture)
		element = new Texture(this, ifile);
		else if (type == EUIScreen)
		element = new UIScreen(this, ifile);
		else if (type == ESkeleton)
		element = new Skeleton(this, ifile);
		else if (type == ESound)
		element = new Sound(this, ifile);*/
		// TODO: add different content types and remove:
		element = new ContentElement(this, ifile);
		return element;
	}

	bool ContentManager::saveElement(uint id)
	{
		lock_guard<mutex> lck(this->thread->mutex("contentMutex"));

		ContentElement* element = this->GetElement(id, false);
		if (!element)
		{
			Scene::Log(EError, "ContentManager", "Cannot save non existent content element (" + to_string(id) + ")");
			return false;
		}

		// Backup
		this->beckupElement(element);

		// Save
		string filePath = string(CONTENT_FOLDER) + string("\\") + element->Package + ".mpk";
		fstream ofile(filePath, ios_base::in | ios_base::out | ios_base::binary);
		if (!ofile || !ofile.is_open())
		{
			ofile.open(filePath, ios_base::out | ios_base::binary);
			if (!ofile || !ofile.is_open())
			{
				Scene::Log(EError, "ContentManager", "Cannot open package file: " + filePath);
				ofile.close();
				return false;
			}
		}
		ofile.seekp(0, ios_base::end);

		// Remove free space
		PackageInfo& info = this->packageInfos[element->Package];
		for (auto it = info.FreeSpaces.begin(); it != info.FreeSpaces.end(); it++)
		{
			if ((*it).second >= element->Size())
			{
				ofile.seekp((*it).first);
				(*it).second -= element->Size();
				if ((*it).second == 0)
					info.FreeSpaces.erase(it);
				break;
			}
		}

		element->PackageOffset = ofile.tellp();
		element->WriteToFile(ofile);
		ofile.close();
		Scene::Log(ELog, "ContentManager", "Save content element '" + element->Name + "'#" +
			to_string(element->Version) + " (" + to_string(element->ID) + ")");

		return true;
	}

	bool ContentManager::eraseElement(uint id)
	{
		lock_guard<mutex> lck(this->thread->mutex("contentMutex"));

		ContentElement* element = this->GetElement(id, false);
		if (!element)
		{
			Scene::Log(EError, "ContentManager", "Cannot erase non existent content element (" + to_string(id) + ")");
			return false;
		}

		// Backup
		this->beckupElement(element);

		// Erase
		string filePath = string(CONTENT_FOLDER) + string("\\") + element->Package + ".mpk";
		fstream ofile(filePath, ios_base::in | ios_base::out | ios_base::binary);
		if (!ofile || !ofile.is_open())
		{
			Scene::Log(EError, "ContentManager", "Cannot open package file: " + filePath);
			ofile.close();
			return false;
		}

		ofile.seekp(element->PackageOffset);
		long long size = element->Size();
		for (int i = 0; i < size; i++)
			ofile.write("\0", sizeof(char));
		ofile.close();

		// Add free space
		PackageInfo& info = this->packageInfos[element->Package];
		bool found = false;
		for (auto it = info.FreeSpaces.begin(); it != info.FreeSpaces.end(); it++)
		{
			if ((*it).first + (*it).second == element->PackageOffset)
			{
				(*it).second += size;
				found = true;
				break;
			}
		}
		if (!found)
			info.FreeSpaces.push_back(make_pair(element->PackageOffset, size));
		sort(info.FreeSpaces.begin(), info.FreeSpaces.end(), [](pair<long long, long long> a, pair<long long, long long> b){ return a.second < b.second; });

		return true;
	}

	void ContentManager::beckupElement(const ContentElement* element)
	{
		string backupPath = BACKUP_FOLDER;
		backupPath += "\\";
		auto now = chrono::system_clock::now();
		time_t time = chrono::system_clock::to_time_t(now);
		tm local_tm;
		localtime_s(&local_tm, &time);
		backupPath += to_string(local_tm.tm_year + 1900) + "-";
		backupPath += to_string(local_tm.tm_mon + 1) + "-";
		backupPath += to_string(local_tm.tm_mday) + "_";
		backupPath += to_string(local_tm.tm_hour) + ".";
		backupPath += to_string(local_tm.tm_min) + ".";
		backupPath += to_string(local_tm.tm_sec) + "_";
		backupPath += element->Package + "_" + element->Name + ".mpk";

		this->thread->mutex("contentMutex").unlock();
		this->ExportToPackage(backupPath, element->ID);
		this->thread->mutex("contentMutex").lock();

		Scene::Log(ELog, "ContentManager", "Backup content element '" + element->Name + "'#" +
			to_string(element->Version) + " (" + to_string(element->ID) + ") to: " + backupPath);
	}

}
