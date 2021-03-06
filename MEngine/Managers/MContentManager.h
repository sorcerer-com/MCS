// MContentManager.h
#pragma once

#include "Engine\Managers\ContentManager.h"
#pragma managed

#include "MBaseManager.h"
#include "..\Utils\MHeader.h"


namespace MyEngine {

	enum class EContentElementType;
	ref class MContentElement;

    public ref class MContentManager : MBaseManager
	{
	private:
        ContentManager* contentManager;

    public:
		property List<String^>^ Paths
		{
			List<String^>^ get();
		}

		property List<MContentElement^>^ Content
		{
			List<MContentElement^>^ get();
        }

#pragma region ContentManager Properties_h
#pragma endregion


		delegate void ChangedEventHandler(MContentManager^ sender, MContentElement^ element);
		event ChangedEventHandler^ Changed;

	public:
        MContentManager(MEngine^ owner, ContentManager* contentManager);

#pragma region ContentManager Functions_h
        bool ImportPackage(String^ filePath);
        bool ExportToPackage(String^ filePath, uint id);
        
        bool CreatePath(String^ fullPath);
        bool RenamePath(String^ oldFullPath, String^ newFullPath);
        bool ContainsPath(String^ fullPath);
        bool DeletePath(String^ fullPath);
        
        MContentElement^ AddElement(EContentElementType type, String^ name, String^ package, String^ path, uint id);
        bool ContainsElement(uint id);
        bool ContainsElement(String^ fullName);
        bool MoveElement(uint id, String^ newFullPath);
        bool DeleteElement(uint id);
        void SaveElement(uint id);
#pragma endregion
        MContentElement^ CloneElement(uint id, String^ newName);
        bool RenameElement(uint id, String^ newName);
        MContentElement^ GetElement(uint id);
        MContentElement^ GetElement(uint id, bool load);
        MContentElement^ GetElement(String^ fullName);
        MContentElement^ GetElement(String^ fullName, bool load);

	private:
		void OnChanged(MContentElement^ element);
		void OnElementChanged(MContentElement ^sender);

		MContentElement^ getMContentElement(const ContentElementPtr& element);

	public:
		static MContentElement^ GetMContentElement(const ContentElementPtr& element);

		static String^ GetPackage(String^ fullName);
		static String^ GetPath(String^ fullName);
		static String^ GetName(String^ fullName);

	};
}
