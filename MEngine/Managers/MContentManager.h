// MContentManager.h
#pragma once

#include "Engine\Managers\ContentManager.h"

#pragma managed

#include "..\Utils\MHeader.h"
#include "..\Content Elements\MContentElement.h"


namespace MEngine {

	public ref class MContentManager : NotifyPropertyChanged
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

	public:
		MContentManager(ContentManager* contentManager);

		bool ImportPackage(String^ filePath);
		bool ExportToPackage(String^ filePath, uint id);

		bool CreatePath(String^ fullPath);
		bool RenamePath(String^ oldFullPath, String^ newFullPath);
		bool DeletePath(String^ fullPath);

		MContentElement^ AddElement(EContentElementType type, String^ name, String^ package, String^ path, uint id);
		MContentElement^ CloneElement(uint id, String^ newName);
		bool RenameElement(uint id, String^ newName);
		bool MoveElement(uint id, String^ newFullPath);
		bool DeleteElement(uint id);
		MContentElement^ GetElement(uint id);
		MContentElement^ GetElement(String^ fullName);
		void SaveElement(uint id);

		static MContentElement^ getMContentElement(ContentElement* element);

	private:

	};
}
