// SceneManager.h
#pragma once

#include "..\Utils\Header.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

    class Engine;
    struct Thread;
	enum SceneElementType;
	class SceneElement;
	class Camera;

	using SceneElementPtr = shared_ptr < SceneElement >;

	class SceneManager
	{
	public:
		using SceneMapType = map < uint, SceneElementPtr >; // id / scene element
		using LayerVectorType = vector < string > ; // layer name

		Camera* ActiveCamera;
		Color4 AmbientLight;
		Color4 FogColor;
        float FogDensity;
        float TimeOfDay;

		Engine* Owner;

    private:
        shared_ptr<Thread> thread;

		SceneMapType sceneElements;
		LayerVectorType layers;
        
	public:
		SceneManager(Engine* owner);

		void New();
		bool Save(const string& filePath);
		bool Load(const string& filePath);

		SceneElementPtr AddElement(SceneElementType type, const string& name, uint contentID, uint id = 0);
		SceneElementPtr AddElement(SceneElementType type, const string& name, const string& contentFullName, uint id = 0);
		bool AddElement(SceneElement* element);
		bool ContainElement(uint id) const;
		bool ContainElement(const string& name) const;
		bool DeleteElement(uint id);
		SceneElementPtr GetElement(uint id);
		SceneElementPtr GetElement(const string& name);
		vector<SceneElementPtr> GetElements() const;

        bool SetTimeOfDay(float hour);

		bool CreateLayer(const string& layer);
		bool RenameLayer(const string& oldLayer, const string& newLayer);
		bool ContainLayer(const string& layer) const;
		bool DeleteLayer(const string& layer);
		vector<string> GetLayers() const;
		vector<SceneElementPtr> GetLayerElements(const string& layer) const;

	};

}