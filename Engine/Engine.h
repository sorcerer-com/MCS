// Engine.h
#pragma once

#include "Utils\Header.h"

namespace std
{
    class mutex;
}
namespace MyEngine {

	class ContentManager;
	class SceneManager;
    class ViewPortRenderer;
    class ProductionRenderer;

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

		static bool IsSelected(uint id);
	};

	class Engine
	{
    private:
        static mutex logMutex;

	public:
		shared_ptr<ContentManager> ContentManager;
		shared_ptr<SceneManager> SceneManager;

        shared_ptr<ViewPortRenderer> ViewPortRenderer;
        shared_ptr<ProductionRenderer> ProductionRenderer;

		static EngineMode Mode;

	public:
		Engine();
		~Engine();

		static void Log(LogType type, const string& category, const string& text);
	};

}
