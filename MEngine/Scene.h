// Scene.h
#pragma once
#pragma unmanaged

#include "Utils\Header.h"


namespace Engine {

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

		static EngineMode Mode;

	public:
		Scene();
		~Scene();


		static void Log(LogType type, const string& category, const string& text);
	};

}
