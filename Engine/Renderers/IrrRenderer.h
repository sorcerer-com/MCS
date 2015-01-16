// IrrRenderer.h
#pragma once

#include "Renderer.h"
#include "..\Utils\Header.h"


namespace irr
{
	namespace video
	{
		class IVideoDriver;
	}

	namespace scene
	{
		class ISceneManager;
	}

	namespace gui
	{
		class IGUIEnvironment;
	}

	class IrrlichtDevice;
}

namespace MyEngine {

	class SceneElement;
	using SceneElementPtr = shared_ptr < SceneElement >;
	
	class IrrRenderer : public Renderer
	{
	private:
		void* windowHandle;
		irr::IrrlichtDevice* device;
		irr::video::IVideoDriver* driver;
		irr::scene::ISceneManager* smgr;
		irr::gui::IGUIEnvironment* guienv;

	public:
		IrrRenderer(Engine* owner);
		~IrrRenderer();

		virtual bool Init(void* params) override;
		virtual void ReSize(int width, int height) override;

	private:
		bool init();
		void updateScene();
		void updateSceneElement(const SceneElementPtr sceneElement);
		void render();

	};

}