// MEngine.h
#pragma once

#include "Engine\Engine.h"
#pragma managed

#include "Managers\MContentManager.h"


namespace MyEngine {

	public enum class EEngineMode
	{
		Editor,
		Engine
	};

	public enum class ELogType
	{
		Log,
		Warning,
		Error
	};

	public ref class MEngine
	{
	private:
		Engine* engine;

	public:
		property MContentManager^ ContentManager;

		static EEngineMode Mode;

	public:
		MEngine();
		~MEngine();

		static void Log(ELogType type, String^ category, String^ text);
	};

}