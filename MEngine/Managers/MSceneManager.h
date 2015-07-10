// MSceneManager.h
#pragma once

#include "Engine\Managers\SceneManager.h"
#pragma managed

#include "MBaseManager.h"
#include "..\Utils\MHeader.h"


namespace MyEngine {

	enum class ESceneElementType;
	ref class MSceneElement;
	ref class MCamera;
	value struct MColor;
    ref class MContentElement;

    public ref class MSceneManager : MBaseManager
	{
	private:
		SceneManager* sceneManager;

	public:
		property List<MSceneElement^>^ Elements
		{
			List<MSceneElement^>^ get();
		}

		property List<String^>^ Layers
		{
			List<String^>^ get();
		}

		property MCamera^ ActiveCamera
		{
			MCamera^ get();
			void set(MCamera^ value);
		}

		property MColor AmbientLight
		{
			MColor get();
			void set(MColor value);
		}

		property MColor FogColor
		{
			MColor get();
			void set(MColor value);
		}

		property double FogDensity
		{
			double get();
			void set(double value);
        }

        property double TimeOfDay
        {
            double get();
            void set(double value);
        }

        property MContentElement^ SkyBox
        {
            MContentElement^ get();
            void set(MContentElement^ value);
        }


		delegate void ChangedEventHandler(MSceneManager^ sender, MSceneElement^ element);
		event ChangedEventHandler^ Changed;

	public:
		MSceneManager(MEngine^ owner, SceneManager* sceneManager);

		void New();
		bool Load(String^ filePath);
		bool Save(String^ filePath);

		MSceneElement^ AddElement(ESceneElementType type, String^ name, uint contentID);
		MSceneElement^ AddElement(ESceneElementType type, String^ name, String^ contentFullName);
		MSceneElement^ CloneElement(uint id, String^ newName);
		bool ContainsElement(uint id);
		bool ContainsElement(String^ name);
		bool RenameElement(String^ oldName, String^ newName);
		bool SetElementLayer(uint id, String^ layer);
		bool DeleteElement(uint id);
		MSceneElement^ GetElement(uint id);
		MSceneElement^ GetElement(String^ name);
        List<MSceneElement^>^ GetElements(ESceneElementType type);
        
		bool CreateLayer(String^ layer);
		bool RenameLayer(String^ oldLayer, String^ newLayer);
		bool ContainsLayer(String^ layer);
		bool DeleteLayer(String^ layer);
		List<MSceneElement^>^ GetLayerElements(String^ layer);

	private:
		void OnChanged(MSceneElement^ element);
		void OnElementChanged(MSceneElement^ sender);

		MSceneElement^ getMSceneElement(const SceneElementPtr& element);

	public:
		static MSceneElement^ GetMSceneElement(const SceneElementPtr& element);
	};

}