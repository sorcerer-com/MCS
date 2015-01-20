// Engine.cpp

#include "stdafx.h"
#include "Engine.h"

#include "Utils\Config.h"
#include "Managers\ContentManager.h"
#include "Managers\SceneManager.h"
#include "Renderers\IrrRenderer.h"


namespace MyEngine {

	/* E N G I N E */
	EngineMode Engine::Mode = EEditor;


	Engine::Engine()
	{
		ofstream ofile(LOG_FILE);
		ofile.close();
		Engine::Log(ELog, "Engine", "Create engine");

		this->ContentManager = make_shared<MyEngine::ContentManager>(this);
		this->SceneManager = make_shared<MyEngine::SceneManager>(this);

		this->ViewPortRenderer = make_shared<IrrRenderer>(this);
	}

	Engine::~Engine()
	{
		Engine::Log(ELog, "Engine", "Destroy engine");
	}


	void Engine::Log(LogType type, const string& category, const string& text)
	{
		if (Engine::Mode == EEngine) // TODO: in other "Record" mode show only errors?
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

		ofile.close();
	}

}