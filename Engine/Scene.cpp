// Scene.cpp

#include "stdafx.h"
#include "Scene.h"

#include <time.h>
#include <fstream>
#include <iomanip>

#include "Utils\Config.h"
#include "Managers\ContentManager.h"


namespace Engine {

	EngineMode Scene::Mode = EEditor;


	Scene::Scene()
	{
		ofstream ofile(LOG_FILE);
		ofile.close();
		Scene::Log(ELog, "Scene", "Start engine");

		this->contentManager = make_shared<ContentManager>();
	}

	Scene::~Scene()
	{

	}


	void Scene::Log(LogType type, const string& category, const string& text)
	{
		if (Scene::Mode != EEditor) 
			return;

		ofstream ofile(LOG_FILE, ios_base::app);
		if (!ofile.is_open())
		{
			ofile.close();
			return;
		}

		string stype = "Log";
		if (type == EWarning)
			stype = "Warning";
		else if (type == EError)
			stype = "Error";

		ofile << left;
		ofile << setw(10) << stype << setw(0);
		ofile << "[" << setw(20) << category << setw(0) << "]: ";
		ofile << text << endl;

		ofile.flush();
		ofile.close();
	}

}