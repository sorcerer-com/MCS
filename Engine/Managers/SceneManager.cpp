// SceneManager.cpp

#include "stdafx.h"
#include "SceneManager.h"

#include "..\Engine.h"
#include "..\Utils\Config.h"
#include "..\Scene Elements\SceneElement.h"


namespace MyEngine {

	/* S C E N E   M A N A G E R */
	SceneManager::SceneManager()
	{
		this->New();
	}


	void SceneManager::New()
	{
		Engine::Log(ELog, "Scene", "New scene");
		this->sceneElements.clear();
	}

	bool SceneManager::Save(const string& filePath)
	{
		Engine::Log(ELog, "Scene", "Save scene to file: " + filePath);
		return true;
	}

	bool SceneManager::Load(const string& filePath)
	{
		Engine::Log(ELog, "Scene", "Load scene from file: " + filePath);
		return true;
	}


	/* E L E M E N T S */
	SceneElementPtr SceneManager::AddElement(SceneElementType type, const string& name, uint contentID, uint id /* = 0 */)
	{
		SceneElement *element = NULL;

		/* TODO: add other scene elements
		if (type == ELight)
			element = new Light(this, name, contentID, EStaticLight);
		else if (type == ECharacter)
			element = new Character(this, name, contentID, INVALID_ID);
		else*/
			element = new SceneElement(this, type, name, contentID);
		element->ID = id;

		if (!this->AddElement(element))
		{
			delete element;
			return SceneElementPtr();
		}
		return this->sceneElements[element->ID];
	}

	bool SceneManager::AddElement(SceneElement* element)
	{
		if (element == NULL)
			throw "ArgumentNullException: element";

		if (element->ID == INVALID_ID)
		{
			do {
				element->ID = (uint)Now;
			} while (this->ContainElement(element->ID));
		}

		if (this->ContainElement(element->ID) || this->GetElement(element->Name))
		{
			Engine::Log(EError, "Scene", "Try to add scene element '" + element->Name + "' (" + to_string(element->ID) + ") that already exists");
			return false;
		}

		this->sceneElements[element->ID] = SceneElementPtr(element);
		Engine::Log(ELog, "Scene", "Add scene element '" + element->Name + "'#" + to_string(element->Version) + " (" + to_string(element->ID) + ")");

		/* if (element->Type != ESystemObject)
			this->AddLayerID("Default", element->ID); */

		return true;
	}

	bool SceneManager::ContainElement(uint id) const
	{
		return this->sceneElements.find(id) != this->sceneElements.end();
	}

	bool SceneManager::DeleteElement(uint id)
	{
		if (!this->ContainElement(id))
		{
			Engine::Log(EError, "Scene", "Try to delete non existent scene element (" + to_string(id) + ")");
			return false;
		}

		//this->RemoveLayerID(id); // remove it from all layers
		//this->ContentManager.ClearInstances(id); // delete all used instaces for this scene element
		this->sceneElements.erase(id);

		Engine::Log(ELog, "Scene", "Delete scene element (" + to_string(id) + ")");
		return true;
	}
	
	SceneElementPtr SceneManager::GetElement(uint id)
	{
		if (!this->ContainElement(id))
		{
			Engine::Log(EError, "Scene", "Try to get non existent scene element (" + to_string(id) + ")");
			return SceneElementPtr();
		}

		return this->sceneElements[id];
	}

	SceneElementPtr SceneManager::GetElement(const string& name)
	{
		SceneMapType::const_iterator it;
		for (it = this->sceneElements.begin(); it != this->sceneElements.end(); it++)
		{
			if ((*it).second->Name.compare(name) == 0)
				return (*it).second;
		}

		Engine::Log(EError, "Scene", "Try to get non existent scene element '" + name + "'");
		return NULL;
	}

}