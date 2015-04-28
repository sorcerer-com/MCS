// IrrRenderer.h
#pragma once

#include "Renderer.h"
#include "..\Utils\Header.h"


namespace irr
{
	namespace video
	{
		class IVideoDriver;
		class SMaterial;
		class ITexture;
		class SColor;
	}

	namespace scene
	{
		class ISceneManager;
		class ISceneNode;
		struct SMesh;
	}

	namespace gui
	{
		class IGUIEnvironment;
	}

	class IrrlichtDevice;
}

namespace MyEngine {

	class ContentElement;
	using ContentElementPtr = shared_ptr < ContentElement >;
	class Material;
	class Texture;
	class SceneElement;
	using SceneElementPtr = shared_ptr < SceneElement >;
	
	class IrrRenderer : public Renderer
	{
	private:
		void* windowHandle;
		irr::IrrlichtDevice* irrDevice;
		irr::video::IVideoDriver* irrDriver;
		irr::scene::ISceneManager* irrSmgr;
		irr::gui::IGUIEnvironment* irrGuienv;

		map<uint, irr::scene::SMesh*> meshesCache;

		static irr::video::SColor irrInvalidColor;

	public:
		IrrRenderer(Engine* owner);
		~IrrRenderer();

		virtual bool Init(void* params) override;
		virtual void ReSize(int width, int height) override;
		virtual uint GetIntesectionInfo(float x, float y, Vector3& dir, Vector3& inter) override;

	private:
		bool init();

		void updateScene();
		void updateSceneElement(const SceneElementPtr sceneElement);

		irr::scene::ISceneNode* createIrrSceneNode(const SceneElementPtr sceneElement);
		bool updateIrrMesh(const SceneElementPtr sceneElement, irr::scene::SMesh* irrMesh);
		bool updateIrrMaterial(const SceneElementPtr sceneElement, irr::video::SMaterial& irrMaterial);
		bool updateIrrTexture(const Material* material, irr::video::ITexture*& irrTexture);
		bool updateIrrTexture(Texture* texture, irr::video::ITexture*& irrTexture);

		void render();

	};

}