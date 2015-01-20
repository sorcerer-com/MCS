// IrrRenderer.cpp

#include "stdafx.h"
#include "IrrRenderer.h"

#pragma warning(push, 3)
#include <Irr\irrlicht.h>
#pragma warning(pop)

#include "..\Engine.h"
#include "..\Utils\Thread.h"
#include "..\Managers\SceneManager.h"
#include "..\Managers\ContentManager.h"
#include "..\Scene Elements\SceneElement.h"
#include "..\Content Elements\ContentElement.h"
#include "..\Content Elements\Mesh.h"


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
		cam->setFOV(irr::core::HALF_PI);

		return true;
	}

	void IrrRenderer::updateScene()
	{
		// TODO: camera

		// TODO: lights, may be they should be normaly updated

		// TODO: fog

		// Update SceneElements
		vector<SceneElementPtr> sceneElements = this->Owner->SceneManager->GetElements();
		for (const auto& sceneElement : sceneElements)
			this->updateSceneElement(sceneElement);
		sceneElements.clear();

		// Remove invalid irrSceneElements
		irr::scene::ISceneNode* irrRootSceneNode = this->smgr->getRootSceneNode();
		const auto irrChildrenSceneNode = irrRootSceneNode->getChildren();
		for (const auto& irrSceneNode : irrChildrenSceneNode)
		{
			if (irrSceneNode && !this->Owner->SceneManager->ContainElement(irrSceneNode->getID()))
				irrSceneNode->remove();
		}

		// Clear meshes cache from unused meshes
		vector<uint> forDelete;
		for (const auto& pair : this->meshesCache)
		{
			if (pair.second->getReferenceCount() == 1)
				forDelete.push_back(pair.first);
		}
		for (const auto& id : forDelete)
			this->meshesCache.erase(id);
	}
	
	void IrrRenderer::updateSceneElement(const SceneElementPtr sceneElement)
	{
		// TODO: skip system objects, cameras, if it's not in editor mode, set them as debug objects

		
		irr::scene::IMeshSceneNode* irrSceneNode = (irr::scene::IMeshSceneNode*)this->smgr->getSceneNodeFromId(sceneElement->ID);
		if (!irrSceneNode)
		{
			if (this->meshesCache.find(sceneElement->ContentID) == this->meshesCache.end())
				this->meshesCache[sceneElement->ContentID] = new irr::scene::SMesh();

			irr::scene::SMesh* mesh = this->meshesCache[sceneElement->ContentID];
			irrSceneNode = this->smgr->addOctreeSceneNode(mesh, NULL, sceneElement->ID);
		}

		irrSceneNode->setName(sceneElement->Name.c_str());
		irrSceneNode->setVisible(sceneElement->Visible); // TODO: if it's not in editor mode else set them half visible

		const Vector3& pos = sceneElement->Position;
		irrSceneNode->setPosition(irr::core::vector3df(pos.x, pos.y, pos.z));
		const Vector3& rot = sceneElement->Rotation.toAxisAngle();
		irrSceneNode->setRotation(irr::core::vector3df(rot.x, rot.y, rot.z));
		const Vector3& scl = sceneElement->Scale;
		irrSceneNode->setScale(irr::core::vector3df(scl.x, scl.y, scl.z));


		// Create mesh
		ContentElementPtr contentElement = this->Owner->ContentManager->GetElement(sceneElement->ContentID, true, true);
		if (!contentElement || contentElement->Type != EMesh)
		{
			Engine::Log(EWarning, "GLRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid mesh (" +
				to_string(sceneElement->ContentID) + ")");
			// TODO: show somehow cube or something and show this only once
			return;
		}
		Mesh* mesh = (Mesh*)contentElement.get();

		irr::scene::SMesh* irrMesh = (irr::scene::SMesh*)irrSceneNode->getMesh();
		irr::scene::SMeshBuffer* irrMeshBuffer = NULL;
		if (irrMesh->getMeshBufferCount() != 0)
			irrMeshBuffer = (irr::scene::SMeshBuffer*)irrMesh->getMeshBuffer(0);
		else
		{
			irrMeshBuffer = new irr::scene::SMeshBuffer();
			irrMesh->addMeshBuffer(irrMeshBuffer);
			irrMeshBuffer->drop();
		}

		// Update mesh
		if ((unsigned int)mesh->Triangles.size() * 3 != irrMeshBuffer->Vertices.size())
		{
			irrMeshBuffer->Vertices.set_used((int)mesh->Triangles.size() * 3);
			irrMeshBuffer->Indices.set_used((int)mesh->Triangles.size() * 3);
			int i = 0;
			for (const auto& triangle : mesh->Triangles)
			{
				for (int j = 0; j < 3; j++)
				{
					irr::video::S3DVertex& v = irrMeshBuffer->Vertices[i];
					const Vector3& pos = mesh->Vertices[triangle.vertices[j]];
					v.Pos = irr::core::vector3df(pos.x, pos.y, pos.z);
					const Vector3& norm = mesh->Normals[triangle.normals[j]];
					v.Normal = irr::core::vector3df(norm.x, norm.y, norm.z);
					const Vector3& tCoord = mesh->TexCoords[triangle.texCoords[j]];
					v.TCoords = irr::core::vector2df(tCoord.x, tCoord.y);

					irrMeshBuffer->Indices[i] = (unsigned short)i;
					i++;
				}
			}
			irrMeshBuffer->recalculateBoundingBox();
			irrMesh->setDirty();
			irrMesh->recalculateBoundingBox();
			irrSceneNode->setMesh(irrMesh);
		}
		irrSceneNode->setMaterialFlag(irr::video::EMF_LIGHTING, false); // TODO: remove
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

			this->updateScene();
			
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