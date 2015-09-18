// SceneManager.h
#pragma once

#include "BaseManager.h"
#include "..\Utils\Header.h"
#include "..\Utils\Types\Color4.h"


namespace MyEngine {

	enum SceneElementType;
	class SceneElement;
	class Camera;

	using SceneElementPtr = shared_ptr < SceneElement >;
	class SceneManager : public BaseManager
	{
	public:
		using SceneMapType = map < uint, SceneElementPtr >; // id / scene element
		using LayerVectorType = vector < string > ; // layer name

		Camera* ActiveCamera;               //* 
		Color4 AmbientLight;                //* default[Color4(0.2f, 0.2f, 0.2f, 1.0f)]
		Color4 FogColor;                    //* default[Color4(0.0f, 0.0f, 0.0f, 1.0f)]
        float FogDensity;                   //* 
        float TimeOfDay;                    //* noproperty
        uint SkyBoxID;                      //* noproperty

    private:
		SceneMapType sceneElements;
		LayerVectorType layers;
        
	public:
        SceneManager(Engine* owner);

        void New();							    //* wrap
        bool Save(const string& filePath) const;//* wrap
        bool Load(const string& filePath);	    //* wrap endgroup

        SceneElementPtr AddElement(SceneElementType type, const string& name, uint contentID, uint id = 0);	//* wrap
        SceneElementPtr AddElement(SceneElementType type, const string& name, const string& contentFullName, uint id = 0);	//* wrap
        bool AddElement(SceneElement* element);	//* wrap
        bool ContainsElement(uint id) const;	//* wrap
        bool ContainsElement(const string& name) const;	//* wrap
        bool DeleteElement(uint id);		    //* wrap
        SceneElementPtr GetElement(uint id);    //* wrap const
        SceneElementPtr GetElement(const string& name);	//* wrap const
        vector<SceneElementPtr> GetElements() const;
        vector<SceneElementPtr> GetElements(SceneElementType type) const;	//* wrap endgroup

        bool SetSkyBox(uint textureID);
        bool SetTimeOfDay(float hour);

        bool CreateLayer(const string& layer);	//* wrap
        bool RenameLayer(const string& oldLayer, const string& newLayer);	//* wrap
        bool ContainsLayer(const string& layer) const;	//* wrap
        bool DeleteLayer(const string& layer);	//* wrap
        vector<string> GetLayers() const;
        vector<SceneElementPtr> GetLayerElements(const string& layer) const;	//* wrap

	};

}