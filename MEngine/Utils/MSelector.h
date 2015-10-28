// MSelection.h
#pragma once

#include "Engine\Engine.h"
#pragma managed
#include "MHeader.h"


namespace MyEngine {

	public ref class MSelector
	{
	public:
		using uint = unsigned int;

		enum class ESelectionType
		{
			ContentElement,
			SceneElement
		};


        delegate void SelectionChangedEventHandler(ESelectionType selectionType, uint id);
        static event SelectionChangedEventHandler^ SelectionChanging;
		static event SelectionChangedEventHandler^ SelectionChanged;


		static List<uint>^ Elements(ESelectionType selectionType)
		{
			List<uint>^ collection = gcnew List<uint>();

			const auto& selection = getSelection(selectionType);
			for (const auto& select : selection)
				collection->Add(select);

			return collection;
		}

		static int Count(ESelectionType selectionType)
		{
			return (int)getSelection(selectionType).size();
		}


		static bool Select(ESelectionType selectionType, uint id)
        {
            SelectionChanging(selectionType, id);
			bool res = getSelection(selectionType).insert(id).second;
			if (res) SelectionChanged(selectionType, id);
			return res;
		}

		static bool Deselect(ESelectionType selectionType, uint id)
		{
            SelectionChanging(selectionType, id);
			bool res = getSelection(selectionType).erase(id) > 0;
			if (res) SelectionChanged(selectionType, id);
			return res;
		}

		static bool IsSelected(ESelectionType selectionType, uint id)
		{
			const auto& selection = getSelection(selectionType);
			return selection.find(id) != selection.end();
		}

		static void Clear(ESelectionType selectionType)
        {
            SelectionChanging(selectionType, 0);
			getSelection(selectionType).clear();
			SelectionChanged(selectionType, 0);
		}

	private:
		static std::set<uint>& getSelection(ESelectionType selectionType)
		{
			if (selectionType == ESelectionType::ContentElement)
				return Selector::ContentElements;
			else if (selectionType == ESelectionType::SceneElement)
				return Selector::SceneElements;
			return Selector::SceneElements;
		}

	};

}