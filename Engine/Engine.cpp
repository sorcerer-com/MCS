// Engine.cpp

#include "stdafx.h"
#include "Engine.h"

#include "Utils\Config.h"
#include "Managers\ContentManager.h"
#include "Managers\SceneManager.h"
#include "Renderers\IrrRenderer.h"


namespace MyEngine {

	/* S E L E C T O R */
	set<uint> Selector::ContentElements;
	set<uint> Selector::SceneElements;


	/* E N G I N E */
	EngineMode Engine::Mode = EngineMode::EEditor;


	Engine::Engine()
	{
		ofstream ofile(LOG_FILE);
		ofile.close();
		Engine::Log(LogType::ELog, "Engine", "Create engine");

		this->ContentManager = make_shared<MyEngine::ContentManager>(this);
		this->SceneManager = make_shared<MyEngine::SceneManager>(this);

		this->ViewPortRenderer = make_shared<IrrRenderer>(this);
	}

	Engine::~Engine()
	{
		Engine::Log(LogType::ELog, "Engine", "Destroy engine");
	}


	void Engine::Log(LogType type, const string& category, const string& text)
	{
		if (Engine::Mode == EngineMode::EEngine) // TODO: in other "Record"/"Movie" mode show only errors?
			return;

		ofstream ofile(LOG_FILE, ios_base::app);
		if (!ofile.is_open())
		{
			ofile.close();
			return;
		}

		string stype = "Log";
		if (type == LogType::EWarning)
			stype = "Warning";
		else if (type == LogType::EError)
			stype = "Error";

		ofile << left;
		ofile << setw(10) << stype << setw(0);
		ofile << "[" << setw(20) << category << setw(0) << "]: ";
		ofile << text << endl;

		ofile.close();
	}

}