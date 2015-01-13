// IrrRenderer.cpp

#include "stdafx.h"
#include "IrrRenderer.h"

#pragma warning(push, 3)
#include <Irr\irrlicht.h>
#pragma warning(pop)

#include "..\Engine.h"
#include "..\Utils\Thread.h"


namespace MyEngine {

	IrrRenderer::IrrRenderer(Engine* owner) :
		Renderer(owner, EIrrRenderer)
	{
		this->device = NULL;
		this->driver = NULL;
		this->smgr = NULL;
	}

	IrrRenderer::~IrrRenderer()
	{
		this->thread->join();

		Engine::Log(ELog, "IrrRenderer", "DeInit IrrLicht Renderer");
	}


	bool IrrRenderer::Init(void* params)
	{
		this->windowHandle = params;

		this->thread->defWorker(&IrrRenderer::render, this);

		Engine::Log(ELog, "IrrRenderer", "Init IrrLicht Renderer");
		return true;
	}

	void IrrRenderer::ReSize(int width, int height)
	{
		this->Width = width;
		this->Height = height;
		this->Resized = true;
		Engine::Log(ELog, "IrrRenderer", "IrrLicht renderer is resized to (" + to_string(width) + ", " + to_string(height) + ")");
	}


	bool IrrRenderer::init()
	{
		irr::SIrrlichtCreationParameters param;
		param.AntiAlias = true;
		param.DriverType = irr::video::EDT_OPENGL;
		param.HandleSRGB = true;
		param.Stencilbuffer = true;
		param.WindowId = this->windowHandle;
		param.ZBufferBits = 32;

		this->device = irr::createDeviceEx(param);
		if (!this->device)
		{
			Engine::Log(EError, "IrrRenderer", "Cannot init IrrLicht Renderer");
			return false;
		}

		this->driver = this->device->getVideoDriver();
		this->smgr = this->device->getSceneManager();
		this->guienv = this->device->getGUIEnvironment();

		// TODO: remove:
		irr::scene::ICameraSceneNode* cam = smgr->addCameraSceneNode();
		cam->setTarget(irr::core::vector3df(0, 0, 0));
		cam->setFOV(irr::core::HALF_PI);

		irr::scene::ISceneNodeAnimator* anim =
			smgr->createFlyCircleAnimator(irr::core::vector3df(0, 15, 0), 30.0f);
		cam->addAnimator(anim);
		anim->drop();

		irr::scene::ISceneNode* cube = smgr->addCubeSceneNode(10);
		cube->setMaterialFlag(irr::video::EMF_LIGHTING, false);
		return true;
	}

	void IrrRenderer::render()
	{
		if (!this->init())
			return;

		while (!this->thread->interrupted())
		{
			if (!this->device->run())
			{
				this_thread::sleep_for(chrono::milliseconds(10));
				continue;
			}

			if (this->Resized)
			{
				this->driver->OnResize(irr::core::dimension2du(this->Width, this->Height));
				this->Resized = false;
			}
			
			this->driver->beginScene();

			this->smgr->drawAll();
			this->guienv->drawAll();

			if (Engine::Mode != EEngine)
			{
				irr::video::SColor white(255, 255, 255, 255);
				this->guienv->getBuiltInFont()->draw(irr::core::stringw("FPS: ") + irr::core::stringw(this->driver->getFPS()), irr::core::recti(10, 10, 0, 0), white);
				this->guienv->getBuiltInFont()->draw(irr::core::stringw("Primitives: ") + irr::core::stringw(this->driver->getPrimitiveCountDrawn()), irr::core::recti(10, 20, 0, 0), white);
			}

			this->driver->endScene();

			this_thread::sleep_for(chrono::milliseconds(1));
		}

		this->device->closeDevice();
		this->device->drop();
	}

}