// Scene.h
#pragma once

#include "Utils\Header.h"


namespace Engine {

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

	class Scene
	{
	public:
		shared_ptr<ContentManager> contentManager;

		static EngineMode Mode;

	public:
		Scene();
		~Scene();


		static void Log(LogType type, const string& category, const string& text);
	};

}
