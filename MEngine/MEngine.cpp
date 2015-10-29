// MEngine.cpp

#include "stdafx.h"
#include "MEngine.h"


namespace MyEngine {

    bool MEngine::IsStarted::get()
    {
        return this->engine->Started;
    }

    void MEngine::IsStarted::set(bool value)
    {
        this->OnStartChanging(value);
        this->engine->Started = value;
        this->OnStartChanged(value);
    }


    EEngineMode MEngine::Mode::get()
    {
        return (EEngineMode)Engine::Mode;
    }

    void MEngine::Mode::set(EEngineMode value)
    {
        Engine::Mode = (EngineMode)value;
    }

    Dictionary<String^, TimeSpan>^ MEngine::ProfilerData::get()
    {
        Dictionary<String^, TimeSpan>^ collection = gcnew Dictionary<String^, TimeSpan>();

        const auto& data = Engine::GetProfilerData();
        for (const auto& pair : data)
        {
            long long milisecs = pair.second;
            long long seconds = pair.second / 1000;
            long long minutes = seconds / 60;
            long long hours = minutes / 60;
            milisecs -= seconds * 1000;
            seconds -= minutes * 60;
            minutes -= hours * 60;

            collection->Add(gcnew String(pair.first.c_str()), TimeSpan(0, (int)hours, (int)minutes, (int)seconds, (int)milisecs));
        }

        return collection;
    }


	MEngine::MEngine()
	{
		this->engine = new Engine();

		this->ContentManager = gcnew MContentManager(this, this->engine->ContentManager.get());
        this->SceneManager = gcnew MSceneManager(this, this->engine->SceneManager.get());
        this->AnimationManager = gcnew MAnimationManager(this, this->engine->AnimationManager.get());

        this->ViewPortRenderer = gcnew MViewPortRenderer(this->engine->ViewPortRenderer.get());
        this->ProductionRenderer = gcnew MProductionRenderer(this->engine->ProductionRenderer.get());
	}

	MEngine::~MEngine()
    {
        delete this->ProductionRenderer;
        this->ProductionRenderer = nullptr;

        delete this->ViewPortRenderer;
        this->ViewPortRenderer = nullptr;

        delete this->AnimationManager;
        this->AnimationManager = nullptr;

        delete this->SceneManager;
        this->SceneManager = nullptr;

		delete this->ContentManager;
		this->ContentManager = nullptr;

		delete this->engine;
		this->engine = NULL;
    }


    void MEngine::Start()
    {
        this->IsStarted = true;
    }

    void MEngine::Stop()
    {
        this->IsStarted = false;
    }


	void MEngine::Log(ELogType type, String^ category, String^ text)
	{
		Engine::Log((LogType)type, to_string(category), to_string(text));
	}


    void MEngine::OnStartChanging(bool value)
    {
        this->StartChanging(this, value);
    }

    void MEngine::OnStartChanged(bool value)
    {
        this->StartChanged(this, value);
    }
}