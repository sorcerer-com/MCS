// Engine.cpp

#include "stdafx.h"
#include "Engine.h"

#include <mutex>

#include "Utils\Config.h"
#include "Utils\Types\Profiler.h"
#include "Managers\ContentManager.h"
#include "Managers\SceneManager.h"
#include "Managers\AnimationManager.h"
#include "Renderers\IrrRenderer.h"
#include "Renderers\CPURayRenderer.h"


namespace MyEngine {

    // P R O F I L E R
    map<string, Profiler::Data> Profiler::data;

	/* S E L E C T O R */
	set<uint> Selector::ContentElements;
	set<uint> Selector::SceneElements;

	bool Selector::IsSelected(uint id)
	{
		if (Selector::ContentElements.find(id) != Selector::ContentElements.end())
			return true;

		if (Selector::SceneElements.find(id) != Selector::SceneElements.end())
			return true;

		return false;
	}


	/* E N G I N E */
    mutex Engine::logMutex;
	EngineMode Engine::Mode = EngineMode::EEditor;


	Engine::Engine()
	{
		ofstream ofile(LOG_FILE);
		ofile.close();
		Engine::Log(LogType::ELog, "Engine", "Create engine");

		this->ContentManager = make_shared<MyEngine::ContentManager>(this);
        this->SceneManager = make_shared<MyEngine::SceneManager>(this);
        this->AnimationManager = make_shared<MyEngine::AnimationManager>(this);

		this->ViewPortRenderer = make_shared<IrrRenderer>(this);
        this->ProductionRenderer = make_shared<CPURayRenderer>(this);
	}

	Engine::~Engine()
	{
        this->ProductionRenderer.reset();
        this->ViewPortRenderer.reset();

        this->AnimationManager.reset();
        this->SceneManager.reset();
        this->ContentManager.reset();

		Engine::Log(LogType::ELog, "Engine", "Destroy engine");
	}


	void Engine::Log(LogType type, const string& category, const string& text)
	{
        lock_guard<mutex> lck(logMutex);
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