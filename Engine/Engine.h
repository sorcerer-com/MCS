// Engine.h
#pragma once

#include "Utils\Header.h"


namespace MyEngine {

	class ContentManager;

	enum LogType
	{
		ELog,
		EWarning,
		EError
	};

	enum EngineMode
	{
		EEditor,
		EEngine
	};

	class Engine
	{
	public:
		shared_ptr<ContentManager> ContentManager;

		static EngineMode Mode;

	public:
		Engine();
		~Engine();


		static void Log(LogType type, const string& category, const string& text);
	};

}
