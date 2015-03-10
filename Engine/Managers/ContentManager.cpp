// ContentManager.cpp

#include "stdafx.h"
#include "ContentManager.h"

#include "..\Engine.h"
#include "..\Utils\Config.h"
#include "..\Utils\Utils.h"
#include "..\Utils\Thread.h"
#include "..\Utils\IOUtils.h"
#include "..\Content Elements\ContentElement.h"
#include "..\Content Elements\Mesh.h"
#include "..\Content Elements\Material.h"


namespace MyEngine {

	/* C O N T E N T   M A N A G E R */
	ContentManager::ContentManager(Engine* owner)
	{
		if (!owner)
			throw "ArgumentNullException: owner";

		this->Owner = owner;

		this->thread = make_shared<Thread>();
		this->thread->defWorker(&ContentManager::doSerilization, this);
		this->thread->defMutex("requests");
		this->thread->defMutex("content", true);

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
			Engine::Log(EError, "ContentManager", "Cannot import package file: " + filePath);
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
			ContentElementType type = ContentElementType::EMesh;
			Read(ifile, type);
			const streamoff offset = -(streamoff)(sizeof(version) + sizeof(type));
			ifile.seekg(offset, ios_base::cur);

			ContentElement* elem = this->loadElement(ifile, type);

			if (elem && !ifile.fail())
				this->AddElement(elem);
			else
			{
				Engine::Log(EError, "ContentManager", "Package file is corrupted: " + filePath);
				ifile.close();
				return false;
			}
		}

		ifile.close();
		Engine::Log(ELog, "ContentManager", "Import package file: " + filePath);
		return true;
	}

	bool ContentManager::ExportToPackage(const string& filePath, uint id)
	{
		if (!this->ContainElement(id))
		{
			Engine::Log(EError, "ContentManager", "Cannot export non existent content element (" + to_string(id) + ")");
			return false;
		}

		// Save
		ofstream ofile(filePath, ios_base::app | ios_base::binary);
		if (!ofile || !ofile.is_open())
		{
			Engine::Log(EWarning, "ContentManager", "Cannot open package file: " + filePath);
			ofile.close();
			return false;
		}

		ContentElement* element = this->GetElement(id, true, true)->Clone();
		element->PackageOffset = ofile.tellp();
		element->WriteToFile(ofile);
		delete element;
		ofile.close();

		return true;
	}


	/* P A T H S */
	bool ContentManager::CreatePath(const string& fullPath)
	{
		if (this->ContainPath(fullPath))
		{
			Engine::Log(EWarning, "ContentManager", "Try to create already exists path '" + fullPath + "'");
			return false;
		}

		string package = ContentManager::GetPackage(fullPath);
		string path = ContentManager::GetPath(fullPath);

		lock lck(this->thread->mutex("content"));
		this->packageInfos[package].Paths[path];

		this->addRequest(ESaveDatabase);

		Engine::Log(ELog, "ContentManager", "Create path '" + fullPath + "'");
		return true;
	}

	bool ContentManager::RenamePath(const string& oldFullPath, const string& newFullPath)
	{
		if (this->ContainPath(newFullPath))
		{
			Engine::Log(EWarning, "ContentManager", "Try to rename non existent path '" + oldFullPath + "'");
			return false;
		}
		
		lock lck(this->thread->mutex("content"));

		string oldPackage = ContentManager::GetPackage(oldFullPath);
		string oldPath = ContentManager::GetPath(oldFullPath);
		PackageInfo& oldInfo = this->packageInfos[oldPackage];

		string newPackage = ContentManager::GetPackage(newFullPath);
		string newPath = ContentManager::GetPath(newFullPath);
		PackageInfo& newInfo = this->packageInfos[newPackage];
			
		vector<string> forDelete;
		for (auto it = oldInfo.Paths.rbegin(); it != oldInfo.Paths.rend(); it++)
		{
			string path = (*it).first;
			if (path.find(oldPath) == 0)
			{
				string npath = path.substr(oldPath.size());
				npath = newPath + npath;

				newInfo.Paths[npath].insert((*it).second.begin(), (*it).second.end());
				forDelete.push_back(path);

				// change elements paths
				const set<uint>& ids = newInfo.Paths[npath];
				for (const auto& id : ids)
				{
					ContentElementPtr element = this->GetElement(id, true, true);
					this->eraseElement(id);
					element->Package = newPackage;
					element->Path = npath;
					element->SavedSize = 0;
					this->addRequest(ESaveElement, id);
					this->addRequest(ESaveDatabase);
				}

				Engine::Log(ELog, "ContentManager", "Rename path '" + oldPackage + "#" + path + "' to '" + newPackage + "#" + npath + "'");
			}
		}
		for (const auto& path : forDelete)
			oldInfo.Paths.erase(path);

		// delete package if it's renamed
		if (oldInfo.Paths.size() == 0)
		{
			this->packageInfos.erase(oldPackage);

			string packFile = CONTENT_FOLDER;
			packFile += "\\" + oldPackage + PACKAGE_EXT;
			remove(packFile.c_str());
		}
			
		this->addRequest(ESaveDatabase);

		return true;
	}

	bool ContentManager::ContainPath(const string& fullPath) const
	{
		string package = ContentManager::GetPackage(fullPath);
		string path = ContentManager::GetPath(fullPath);
		
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
			Engine::Log(EWarning, "ContentManager", "Try to delete non existent path '" + fullPath + "'");
			return false;
		}

		lock lck(this->thread->mutex("content"));
		string package = ContentManager::GetPackage(fullPath);
		string path = ContentManager::GetPath(fullPath);
		PackageInfo& info = this->packageInfos[package];

		vector<string> forDelete;
		for (const auto& pair : info.Paths)
		{
			string currPath = pair.first;
			if (currPath.find(path) == 0)
			{
				set<uint> ids = info.Paths[currPath];
				for (const auto& id : ids)
					this->DeleteElement(id);

				forDelete.push_back(currPath);
			}
		}

		for (const auto& path : forDelete)
			info.Paths.erase(path);

		// delete package if it's deleted
		if (info.Paths.size() == 0)
		{
			this->packageInfos.erase(package);

			string packFile = CONTENT_FOLDER;
			packFile += "\\" + package + PACKAGE_EXT;
			remove(packFile.c_str());
		}
		this->addRequest(ESaveDatabase);

		Engine::Log(ELog, "ContentManager", "Delete path '" + fullPath + "'");
		return true;
	}
	
	vector<string> ContentManager::GetPaths() const
	{
		vector<string> result;
		for (const auto& pair : this->packageInfos)
		{
			for (const auto& pair2 : pair.second.Paths)
				result.push_back(pair.first + "#" + pair2.first + "\\");
		}

		return result;
	}


	/* E L E M E N T S */
	ContentElementPtr ContentManager::AddElement(ContentElementType type, const string& name, const string& package, const string& path, uint id /* = 0 */)
	{
		ContentElement *element = NULL;
		if (type == ContentElementType::EMesh)
			element = new Mesh(this, name, package, path);
		else if (type == EMaterial)
			element = new Material(this, name, package, path);
		/* TODO: add different content types
		else if (type == ETexture)
			element = new Texture(this, name, package, path);
		else if (type == EUIScreen)
			element = new UIScreen(this, name, package, path);
		else if (type == ESkeleton)
			element = new Skeleton(this, name, package, path);
		else if (type == ESound)
			element = new Sound(this, name, package, path);*/

		if (element)
		{
			element->ID = id;
			if (!this->AddElement(element))
			{
				delete element;
				return ContentElementPtr();
			}
		}
		return this->content[element->ID];
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

		if (this->ContainElement(element->ID) || this->ContainElement(element->GetFullName()))
			Engine::Log(EWarning, "ContentManager", "Add content element '" + element->GetFullName() + "' (" + to_string(element->ID) + ") that already exists");

		lock lck(this->thread->mutex("content"));
		this->content[element->ID] = ContentElementPtr(element);
		this->packageInfos[element->Package].Paths[element->Path].insert(element->ID);

		this->addRequest(ESaveElement, element->ID);
		this->addRequest(ESaveDatabase);

		Engine::Log(ELog, "ContentManager", "Add content element '" + element->Name + "'#" + to_string(element->Version) + " (" + to_string(element->ID) + ")");
		return true;
	}

	bool ContentManager::MoveElement(uint id, const string& newFullPath)
	{
		if (!this->ContainElement(id))
		{
			Engine::Log(EWarning, "ContentManager", "Try to move non existent content element (" + to_string(id) + ")");
			return false;
		}
		if (!this->ContainPath(newFullPath))
		{
			Engine::Log(EWarning, "ContentManager", "Try to move element to non existent path '" + newFullPath + "'");
			return false;
		}

		ContentElementPtr element = this->GetElement(id, true, true);
		if (this->ContainElement(newFullPath + element->Name))
		{
			Engine::Log(EWarning, "ContentManager", "Try to move content element '" + element->Name + "' (" + to_string(element->ID) +
				") to path '" + newFullPath + "', but there is already element with the same name");
			return false;
		}

		lock lck(this->thread->mutex("content"));
		set<uint>& ids = this->packageInfos[element->Package].Paths[element->Path];
		ids.erase(ids.find(id));
		element->Package = ContentManager::GetPackage(newFullPath);
		element->Path = ContentManager::GetPath(newFullPath);
		this->packageInfos[element->Package].Paths[element->Path].insert(element->ID);

		this->addRequest(ESaveElement, id);
		this->addRequest(ESaveDatabase);

		Engine::Log(ELog, "ContentManager", "Move content element '" + element->Name + "' (" + to_string(id) + ") to '" + newFullPath + "'");
		return true;
	}

	bool ContentManager::ContainElement(uint id) const
	{
		return this->content.find(id) != this->content.end();
	}

	bool ContentManager::ContainElement(const string& fullname)
	{
		string package = ContentManager::GetPackage(fullname);
		string path = ContentManager::GetPath(fullname);
		string name = ContentManager::GetName(fullname);

		if (this->packageInfos.find(package) == this->packageInfos.end())
			return false;

		PackageInfo& info = this->packageInfos[package];

		if (info.Paths.find(path) == info.Paths.end())
			return false;

		set<uint>& ids = info.Paths[path];
		for (const auto& id : ids)
		{
			ContentElementPtr elem = this->GetElement(id, false);
			if (elem->Name.compare(name) == 0)
				return true;
		}

		return false;
	}

	bool ContentManager::DeleteElement(uint id)
	{
		if (!this->ContainElement(id))
		{
			Engine::Log(EWarning, "ContentManager", "Try to delete non existent content element (" + to_string(id) + ")");
			return false;
		}

		lock lck(this->thread->mutex("content"));
		ContentElementPtr element = this->GetElement(id, false);
		set<uint>& ids = this->packageInfos[element->Package].Paths[element->Path];
		ids.erase(ids.find(id));
		this->eraseElement(id);
		this->content.erase(id);

		this->addRequest(ESaveDatabase);

		Engine::Log(ELog, "ContentManager", "Delete content element (" + to_string(id) + ")");
		return true;
	}

	ContentElementPtr ContentManager::GetElement(uint id, bool load, bool waitForLoad /* = false */)
	{
		if (!this->ContainElement(id))
		{
			Engine::Log(EWarning, "ContentManager", "Try to get non existent content element (" + to_string(id) + ")");
			return ContentElementPtr();
		}

		// load element if it isn't
		if (load)
		{
			if (waitForLoad)
				this->loadElement(id);
			else
				this->addRequest(ELoadElement, id);
		}

		return this->content[id];
	}

	ContentElementPtr ContentManager::GetElement(const string& fullname, bool load, bool waitForLoad /* = false */)
	{
		string package = ContentManager::GetPackage(fullname);
		string path = ContentManager::GetPath(fullname);
		string name = ContentManager::GetName(fullname);

		if (this->packageInfos.find(package) == this->packageInfos.end())
		{
			Engine::Log(EWarning, "ContentManager", "Try to get non existent content element '" + path + "'");
			return ContentElementPtr();
		}

		PackageInfo& info = this->packageInfos[package];

		if (info.Paths.find(path) == info.Paths.end())
		{
			Engine::Log(EWarning, "ContentManager", "Try to get non existent content element '" + path + "'");
			return ContentElementPtr();
		}

		set<uint>& ids = info.Paths[path];
		for (const auto& id : ids)
		{
			ContentElementPtr elem = this->GetElement(id, false);
			if (elem->Name.compare(name) == 0)
				return this->GetElement(id, load, waitForLoad);
		}

		return ContentElementPtr();
	}
	
	vector<ContentElementPtr> ContentManager::GetElements()
	{
		vector<ContentElementPtr> result;
		for (const auto& pair : this->content)
			result.push_back(pair.second);

		return result;
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
				this->thread->mutex("requests").lock();
				auto request = this->requests.front();
				this->thread->mutex("requests").unlock();

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
					this->eraseElement(request.second, false);
					this->saveElement(request.second);
					break;
				default:
					Engine::Log(EWarning, "ContentManager", "Invalid request");
					break;
				}

				this->thread->mutex("requests").lock();
				this->requests.pop_front();
				this->thread->mutex("requests").unlock();

				this_thread::sleep_for(chrono::milliseconds(1));
			}

			this_thread::sleep_for(chrono::milliseconds(100));

			// unload all unused content elements
			if (Engine::Mode != EngineMode::EEditor)
			{
				lock lck(this->thread->mutex("content"));
				vector<uint> forUnload;
				for (const auto& pair : this->content)
				{
					if (pair.second.unique() && pair.second->IsLoaded)
					{
						// check if element is in request
						bool inRequest = false;
						this->thread->mutex("requests").lock();
						for (const auto& pair2 : this->requests)
						{
							if (pair2.second == pair.first)
								inRequest = true;
						}
						this->thread->mutex("requests").unlock();

						if (!inRequest)
							forUnload.push_back(pair.first);
					}
				}
				for (const auto& id : forUnload)
					this->unLoadElement(id);
			}
		}
	}

	void ContentManager::addRequest(RequestType type, uint id /* = 0 */)
	{
		lock lck(this->thread->mutex("requests"));

		auto pair = make_pair(type, id);
		auto it = find(this->requests.begin(), this->requests.end(), pair);

		bool skip = false;
		if ((type != RequestType::ESaveDatabase && it != this->requests.end()) || 
			(type == RequestType::ESaveDatabase && this->requests.size() > 0 && this->requests.back() == pair))
			skip = true;

		if (!skip)
			this->requests.push_back(pair);
	}


	void ContentManager::loadDatabase()
	{
		lock lck(this->thread->mutex("content"));

		string filePath = string(CONTENT_FOLDER) + string("\\") + string(CONTENT_DB_FILE);
		Engine::Log(ELog, "ContentManager", "Load database file: " + filePath);

		ifstream ifile(filePath, ios_base::in | ios_base::binary);
		if (!ifile || !ifile.is_open())
		{
			Engine::Log(EError, "ContentManager", "Cannot load database file: " + filePath);
			ifile.close();
			return;
		}

		// Package Infos
		int size = 0;
		Read(ifile, size);
		for (int i = 0; i < size; i++)
		{
			string packageName;
			Read(ifile, packageName);
			PackageInfo info;

			string str;
			int size2 = 0;
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
				this->content[element->ID] = ContentElementPtr(element);
				this->packageInfos[element->Package].Paths[element->Path].insert(element->ID);
			}
		}

		ifile.close();
	}

	void ContentManager::saveDatabase()
	{
		lock lck(this->thread->mutex("content"));

		string filePath = string(CONTENT_FOLDER) + string("\\") + string(CONTENT_DB_FILE);
		Engine::Log(ELog, "ContentManager", "Save database file: " + filePath);

		// Save
		ofstream ofile(filePath, ios_base::out | ios_base::binary);
		if (!ofile || !ofile.is_open())
		{
			Engine::Log(EError, "ContentManager", "Cannot create database file: " + filePath);
			ofile.close();
			return;
		}

		// Package Infos
		Write(ofile, (int)this->packageInfos.size());
		for (const auto& pair : this->packageInfos)
		{
			Write(ofile, pair.first);
			const PackageInfo& info = pair.second;

			Write(ofile, (int)info.Paths.size());
			for (const auto& pair2 : info.Paths)
				Write(ofile, pair2.first);

			Write(ofile, (int)info.FreeSpaces.size());
			for (const auto& pair2 : info.FreeSpaces)
			{
				Write(ofile, pair2.first);
				Write(ofile, pair2.second);
			}
		}
		
		// Content Elements
		Write(ofile, (int)this->content.size());
		for (const auto& pair : this->content)
		{
			ContentElementPtr element = pair.second;
			long long savedSize = element->SavedSize;
			element->ContentElement::WriteToFile(ofile);
			element->SavedSize = savedSize;
		}

		ofile.close();
	}

	bool ContentManager::loadElement(uint id)
	{
		lock lck(this->thread->mutex("content"));

		ContentElementPtr element = this->GetElement(id, false);
		if (!element)
		{
			Engine::Log(EError, "ContentManager", "Cannot load non existent content element (" + to_string(id) + ")");
			return false;
		}

		if (element->IsLoaded)
			return true;

		// Load
		string filePath = string(CONTENT_FOLDER) + string("\\") + element->Package + PACKAGE_EXT;
		ifstream ifile(filePath, ios_base::in | ios_base::binary);
		if (!ifile || !ifile.is_open())
		{
			Engine::Log(EError, "ContentManager", "Cannot load package file: " + filePath);
			ifile.close();
			return false;
		}

		ifile.seekg(element->PackageOffset);

		ContentElement* elem = this->loadElement(ifile, element->Type);

		if (elem && !ifile.fail())
			this->content[elem->ID].reset(elem);
		else
		{
			Engine::Log(EError, "ContentManager", "Cannot load content element '" + element->Name + "'#" +
				to_string(element->Version) + " (" + to_string(element->ID) + ")");
			ifile.close();
			return false;
		}

		ifile.close();
		Engine::Log(ELog, "ContentManager", "Load content element '" + elem->Name + "'#" +
			to_string(elem->Version) + " (" + to_string(elem->ID) + ")");
		return true;
	}

	ContentElement* ContentManager::loadElement(istream& ifile, ContentElementType type)
	{
		ContentElement* element = NULL;
		if (type == ContentElementType::EMesh)
			element = new Mesh(this, ifile);
		else if (type == EMaterial)
			element = new Material(this, ifile);
		/* TODO: add different content types
		else if (type == ETexture)
			element = new Texture(this, ifile);
		else if (type == EUIScreen)
			element = new UIScreen(this, ifile);
		else if (type == ESkeleton)
			element = new Skeleton(this, ifile);
		else if (type == ESound)
			element = new Sound(this, ifile);*/
		return element;
	}

	bool ContentManager::saveElement(uint id, bool backup /* = true */)
	{
		lock lck(this->thread->mutex("content"));

		ContentElementPtr element = this->GetElement(id, false);
		if (!element)
		{
			Engine::Log(EError, "ContentManager", "Cannot save non existent content element (" + to_string(id) + ")");
			return false;
		}

		// Backup
		if (backup)
			this->beckupElement(element, false);

		// Save
		string filePath = string(CONTENT_FOLDER) + string("\\") + element->Package + PACKAGE_EXT;
		fstream ofile(filePath, ios_base::in | ios_base::out | ios_base::binary);
		if (!ofile || !ofile.is_open())
		{
			ofile.open(filePath, ios_base::out | ios_base::binary);
			if (!ofile || !ofile.is_open())
			{
				Engine::Log(EError, "ContentManager", "Cannot open package file: " + filePath);
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
				(*it).first += element->Size();
				if ((*it).second == 0)
					info.FreeSpaces.erase(it);
				break;
			}
		}

		element->PackageOffset = ofile.tellp();
		element->WriteToFile(ofile);
		ofile.close();
		Engine::Log(ELog, "ContentManager", "Save content element '" + element->Name + "'#" +
			to_string(element->Version) + " (" + to_string(element->ID) + ")");

		return true;
	}

	bool ContentManager::eraseElement(uint id, bool backup /* = true */)
	{
		lock lck(this->thread->mutex("content"));

		ContentElementPtr element = this->GetElement(id, false);
		if (!element)
		{
			Engine::Log(EError, "ContentManager", "Cannot erase non existent content element (" + to_string(id) + ")");
			return false;
		}

		long long size = element->SavedSize;
		if (size == 0)
			return true;

		// Backup
		if (backup)
			this->beckupElement(element, true);

		// Erase
		string filePath = string(CONTENT_FOLDER) + string("\\") + element->Package + PACKAGE_EXT;
		fstream ofile(filePath, ios_base::in | ios_base::out | ios_base::binary);
		if (!ofile || !ofile.is_open())
		{
			Engine::Log(EError, "ContentManager", "Cannot open package file: " + filePath);
			ofile.close();
			return false;
		}

		ofile.seekp(element->PackageOffset);
		for (int i = 0; i < size; i++)
			ofile.write("\0", sizeof(char));
		ofile.close();

		// Add free space
		PackageInfo& info = this->packageInfos[element->Package];
		bool found = false;
		for (auto& pair : info.FreeSpaces)
		{
			if (pair.first + pair.second == element->PackageOffset)
				found = true;
			if (element->PackageOffset + size == pair.first)
			{
				pair.first = element->PackageOffset;
				found = true;
			}

			if (found)
			{
				pair.second += size;
				break;
			}
		}
		if (!found)
			info.FreeSpaces.push_back(make_pair(element->PackageOffset, size));
		sort(info.FreeSpaces.begin(), info.FreeSpaces.end(), [](const pair<long long, long long>& a, const pair<long long, long long>& b){ return a.second < b.second; });

		Engine::Log(ELog, "ContentManager", "Erase content element '" + element->Name + "'#" +
			to_string(element->Version) + " (" + to_string(element->ID) + ")");
		return true;
	}

	void ContentManager::beckupElement(const ContentElementPtr& element, bool erase)
	{
		if (Engine::Mode == EngineMode::EEngine)
			return;

		string backupPath = BACKUP_FOLDER + string("\\");
		backupPath += dateTimeFileName();
		backupPath += "_" + element->Package + "_" + element->Name;
		if (erase) backupPath += "_erase";
		backupPath += PACKAGE_EXT;

		this->ExportToPackage(backupPath, element->ID);

		Engine::Log(ELog, "ContentManager", "Backup content element '" + element->Name + "'#" +
			to_string(element->Version) + " (" + to_string(element->ID) + ") to: " + backupPath);
	}

	bool ContentManager::unLoadElement(uint id)
	{
		ContentElementPtr element = this->GetElement(id, false);
		if (!element)
		{
			Engine::Log(EError, "ContentManager", "Cannot unload non existent content element (" + to_string(id) + ")");
			return false;
		}

		if (!element->IsLoaded)
			return true;

		ContentElement* elem = new ContentElement(*element);
		elem->IsLoaded = false;
		lock lck(this->thread->mutex("content"));
		this->content[elem->ID].reset(elem);

		Engine::Log(ELog, "ContentManager", "UnLoad content element '" + elem->Name + "'#" +
			to_string(elem->Version) + " (" + to_string(elem->ID) + ")");
		return true;
	}


	string ContentManager::GetPackage(const string& fullName)
	{
		return fullName.substr(0, fullName.find_last_of("#"));
	}

	string ContentManager::GetPath(const string& fullName)
	{
		size_t start = fullName.find_last_of("#") + 1;
		size_t end = fullName.find_last_of("\\");
		if (end != string::npos)
			return fullName.substr(start, end - start);
		else
			return "";
	}

	string ContentManager::GetName(const string& fullName)
	{
		size_t slash = fullName.find_last_of("\\");
		size_t hash = fullName.find_last_of("#");
		if (slash != string::npos)
		{
			if (slash > hash)
				return fullName.substr(slash + 1);
			else
				return "";
		}
		else
			return fullName.substr(hash + 1);
	}

}
