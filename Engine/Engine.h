// Engine.h
#pragma once

#include "Utils\Header.h"


namespace MyEngine {

	class ContentManager;
	class SceneManager;
	class Renderer;

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

	class Selector
	{
	public:
		static set<uint> ContentElements;
		static set<uint> SceneElements;
	};

	class Engine
	{
	public:
		shared_ptr<ContentManager> ContentManager;
		shared_ptr<SceneManager> SceneManager;

		shared_ptr<Renderer> ViewPortRenderer;

		static EngineMode Mode;

	public:
		Engine();
		~Engine();


		static void Log(LogType type, const string& category, const string& text);
	};

}
