// Scene.h
#pragma once
#pragma unmanaged

#include <string>

using namespace std;


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
