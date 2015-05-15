// SceneManager.cpp

#include "stdafx.h"
#include "SceneManager.h"

#include "..\Engine.h"
#include "..\Utils\Config.h"
#include "..\Utils\Utils.h"
#include "..\Utils\Thread.h"
#include "..\Utils\IOUtils.h"
#include "..\Scene Elements\SceneElement.h"
#include "..\Scene Elements\Camera.h"
#include "..\Scene Elements\Light.h"

#include "..\Managers\ContentManager.h"
#include "..\Content Elements\ContentElement.h"


namespace MyEngine {

	/* S C E N E   M A N A G E R */
	SceneManager::SceneManager(Engine* owner)
	{
		if (!owner)
			throw "ArgumentNullException: owner";

        this->Owner = owner;

        this->thread = make_shared<Thread>();
        this->thread->defMutex("content", true);

		this->New();
	}


	void SceneManager::New()
    {
        lock lck(this->thread->mutex("content"));

		Engine::Log(LogType::ELog, "Scene", "New scene");
		this->sceneElements.clear();
		this->layers.clear();
		this->layers.push_back(DEFAULT_LAYER_NAME);
		this->ActiveCamera = NULL;
		this->AmbientLight = Color4(0.2, 0.2, 0.2, 1.0);
		this->FogColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);
		this->FogDensity = 0.0f;
	}

	bool SceneManager::Save(const string& filePath)
    {
        lock lck(this->thread->mutex("content"));

		// Backup
		if (Engine::Mode != EngineMode::EEngine)
		{
			string fileName = filePath.substr(filePath.find_last_of("\\") + 1, filePath.find_last_of(".") - filePath.find_last_of("\\") - 1);
			string backupPath = BACKUP_FOLDER + string("\\");
			backupPath += dateTimeFileName();
			backupPath += "_" + fileName + SCENE_EXT;

			ifstream src(filePath.c_str(), std::ios::binary);
			if (src.is_open())
			{
				ofstream dst(backupPath.c_str(), std::ios::binary);
				dst << src.rdbuf();

				Engine::Log(LogType::ELog, "Scene", "Backup scene file: " + filePath + " to: " + backupPath);
			}
		}

		// Save
		ofstream ofile(filePath, ios_base::out | ios_base::binary);
		if (!ofile || !ofile.is_open())
		{
			Engine::Log(LogType::EError, "Scene", "Cannot save scene file: " + filePath);
			ofile.close();
			return false;
		}

		// version
		Write(ofile, CURRENT_VERSION);

		// scene elements
		Write(ofile, (int)this->sceneElements.size());
		for (const auto& sce : this->sceneElements)
			sce.second->WriteToFile(ofile);

		// Layers
		Write(ofile, this->layers);

		// active camera name
		if (this->ActiveCamera)
			Write(ofile, this->ActiveCamera->ID);
		else
			Write(ofile, INVALID_ID);

		// ambient light
		Write(ofile, this->AmbientLight);

		// fog
		Write(ofile, this->FogColor);
		Write(ofile, this->FogDensity);

		ofile.close();

		Engine::Log(LogType::ELog, "Scene", "Save scene to file: " + filePath);
		return true;
	}

	bool SceneManager::Load(const string& filePath)
	{
        this->New();
        lock lck(this->thread->mutex("content"));

		ifstream ifile(filePath, ios_base::in | ios_base::binary);
		if (!ifile || !ifile.is_open())
		{
			Engine::Log(LogType::EError, "Scene", "Cannot load scene file: " + filePath);
			ifile.close();
			return false;
		}

		// version
		int ver = CURRENT_VERSION;
		Read(ifile, ver);
		if (ver >= 1)
		{
			// scene elements count
			int size = 0;
			Read(ifile, size);

			// scene elements
			for (int i = 0; i < size; i++)
			{
				uint version = 0u;
				Read(ifile, version);
				if (version == 0u)
				{
					Engine::Log(LogType::EError, "Scene", "Scene file is corrupted");
					ifile.close();
					return false;
				}

				SceneElementType type = SceneElementType::ECamera;
				Read(ifile, type);
				const streamoff offset = -(streamoff)(sizeof(version) + sizeof(type));
				ifile.seekg(offset, ios_base::cur);

				SceneElement* element = NULL;
				if (type == SceneElementType::ECamera)
					element = new Camera(this, ifile);
				else if (type == SceneElementType::ELight)
					element = new Light(this, ifile);
				/* TODO: add different scene elements types */
				else
					element = new SceneElement(this, ifile);

				if (element && !ifile.fail())
					this->AddElement(element);
				else
				{
					Engine::Log(LogType::EError, "Scene", "Scene file is corrupted");
					ifile.close();
					return false;
				}
			}

			// Layers
			Read(ifile, this->layers);

			// active camera name
			uint cameraId;
			Read(ifile, cameraId);
			if (cameraId != INVALID_ID)
				this->ActiveCamera = (Camera*)this->GetElement(cameraId).get();

			// ambient light
			Read(ifile, this->AmbientLight);

			// fog
			Read(ifile, this->FogColor);
			Read(ifile, this->FogDensity);
		}

		ifile.close();

		Engine::Log(LogType::ELog, "Scene", "Load scene from file: " + filePath);
		return true;
	}


	/* E L E M E N T S */
	SceneElementPtr SceneManager::AddElement(SceneElementType type, const string& name, uint contentID, uint id /* = 0 */)
	{
		SceneElement *element = NULL;

		if (type == SceneElementType::ECamera)
			element = new Camera(this, name, contentID);
		else if (type == SceneElementType::ELight)
			element = new Light(this, name, contentID, EStaticLight);
		/* TODO: add other scene elements */
		else
			element = new SceneElement(this, type, name, contentID);
		element->ID = id;

		if (!this->AddElement(element))
		{
			delete element;
			return SceneElementPtr();
		}
		return this->sceneElements[element->ID];
	}
	
	SceneElementPtr SceneManager::AddElement(SceneElementType type, const string& name, const string& contentFullName, uint id /* = 0 */)
	{
		ContentElementPtr celem = this->Owner->ContentManager->GetElement(contentFullName, false);
		uint contentID = 0;
		if (celem)
			contentID = celem->ID;

		return this->AddElement(type, name, contentID, id);
	}

	bool SceneManager::AddElement(SceneElement* element)
	{
		if (!element)
			throw "ArgumentNullException: element";

		if (element->ID == INVALID_ID)
		{
			do {
				element->ID = (uint)Now;
			} while (this->ContainElement(element->ID));
		}

		if (this->ContainElement(element->ID) || this->ContainElement(element->Name))
		{
			Engine::Log(LogType::EWarning, "Scene", "Try to add scene element '" + element->Name + "' (" + to_string(element->ID) + ") that already exists");
			return false;
		}

        lock lck(this->thread->mutex("content"));
		this->sceneElements[element->ID] = SceneElementPtr(element);
		Engine::Log(LogType::ELog, "Scene", "Add scene element '" + element->Name + "'#" + to_string(element->Version) + " (" + to_string(element->ID) + ")");

		return true;
	}

	bool SceneManager::ContainElement(uint id) const
	{
		return this->sceneElements.find(id) != this->sceneElements.end();
	}

	bool SceneManager::ContainElement(const string& name) const
	{
		for (const auto& pair : this->sceneElements)
		{
			if (pair.second->Name.compare(name) == 0)
				return true;
		}

		return false;
	}

	bool SceneManager::DeleteElement(uint id)
	{
		if (!this->ContainElement(id))
		{
			Engine::Log(LogType::EWarning, "Scene", "Try to delete non existent scene element (" + to_string(id) + ")");
			return false;
		}

        lock lck(this->thread->mutex("content"));
		//this->ContentManager.ClearInstances(id); // delete all used instaces for this scene element
		this->sceneElements.erase(id);

		Engine::Log(LogType::ELog, "Scene", "Delete scene element (" + to_string(id) + ")");
		return true;
	}
	
	SceneElementPtr SceneManager::GetElement(uint id)
	{
		if (!this->ContainElement(id))
		{
			Engine::Log(LogType::EWarning, "Scene", "Try to get non existent scene element (" + to_string(id) + ")");
			return SceneElementPtr();
		}

		return this->sceneElements[id];
	}

	SceneElementPtr SceneManager::GetElement(const string& name)
	{
		for (const auto& pair : this->sceneElements)
		{
			if (pair.second->Name.compare(name) == 0)
				return pair.second;
		}

		Engine::Log(LogType::EWarning, "Scene", "Try to get non existent scene element '" + name + "'");
		return NULL;
	}

	vector<SceneElementPtr> SceneManager::GetElements() const
    {
		vector<SceneElementPtr> result;
		for (const auto& pair : this->sceneElements)
			result.push_back(pair.second);

		return result;
	}


	/* L A Y E R S */
	bool SceneManager::CreateLayer(const string& layer)
	{
		if (this->ContainLayer(layer))
		{
			Engine::Log(LogType::EWarning, "Scene", "Try to create already exists layer '" + layer + "'");
			return false;
		}

        lock lck(this->thread->mutex("content"));
		this->layers.push_back(layer);
		Engine::Log(LogType::ELog, "Scene", "Create layer '" + layer + "'");
		return true;
	}

	bool SceneManager::RenameLayer(const string& oldLayer, const string& newLayer)
	{
		if (oldLayer == DEFAULT_LAYER_NAME)
		{
			Engine::Log(LogType::EWarning, "Scene", "Try to rename 'Default' layer");
			return false;
		}
		if (!this->ContainLayer(oldLayer))
		{
			Engine::Log(LogType::EWarning, "Scene", "Try to rename non existent layer '" + oldLayer + "'");
			return false;
		}
		if (this->ContainLayer(newLayer))
		{
			Engine::Log(LogType::EWarning, "Scene", "Try to rename layer '" + oldLayer + "' to '" + newLayer +
				"', but already exists one with that name");
			return false;
		}

        lock lck(this->thread->mutex("content"));
		for (const auto& pair : this->sceneElements)
			if (pair.second->Layer == oldLayer)
				pair.second->Layer = newLayer;

		const auto& it = find(this->layers.begin(), this->layers.end(), oldLayer);
		this->layers.erase(it);
		this->layers.push_back(newLayer);
		Engine::Log(LogType::ELog, "Scene", "Rename layer '" + oldLayer + "' to '" + newLayer + "'");
		return true;
	}

	bool SceneManager::ContainLayer(const string& layer) const
	{
		return find(this->layers.begin(), this->layers.end(), layer) != this->layers.end();
	}

	bool SceneManager::DeleteLayer(const string& layer)
	{
		if (layer == DEFAULT_LAYER_NAME)
		{
			Engine::Log(LogType::EWarning, "Scene", "Try to delete 'Default' layer");
			return false;
		}
		if (!this->ContainLayer(layer))
		{
			Engine::Log(LogType::EWarning, "Scene", "Try to delete non existent layer '" + layer + "'");
			return false;
		}

        lock lck(this->thread->mutex("content"));
		for (const auto& pair : this->sceneElements)
			if (pair.second->Layer == layer)
				pair.second->Layer = DEFAULT_LAYER_NAME;

		const auto& it = find(this->layers.begin(), this->layers.end(), layer);
		this->layers.erase(it);

		Engine::Log(LogType::ELog, "Scene", "Delete layer '" + layer + "'");
		return true;
	}

	vector<string> SceneManager::GetLayers() const
    {
		vector<string> result;
		for (const auto& pair : this->layers)
			result.push_back(pair);

		return result;
	}

	vector<SceneElementPtr> SceneManager::GetLayerElements(const string& layer) const
	{
		vector<SceneElementPtr> result;
		if (!this->ContainLayer(layer))
		{
			Engine::Log(LogType::EWarning, "Scene", "Try to get elements in non existent layer '" + layer + "'");
			return result;
		}

		for (const auto& pair : this->sceneElements)
			if (pair.second->Layer == layer)
				result.push_back(pair.second);

		return result;
	}

}