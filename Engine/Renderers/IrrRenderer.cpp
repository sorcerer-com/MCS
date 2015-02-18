// IrrRenderer.cpp

#include "stdafx.h"
#include "IrrRenderer.h"

#pragma warning(push, 3)
#include <Irr\irrlicht.h>
#pragma warning(pop)

#include "..\Engine.h"
#include "..\Utils\Thread.h"
#include "..\Utils\Config.h"
#include "..\Managers\SceneManager.h"
#include "..\Managers\ContentManager.h"
#include "..\Scene Elements\SceneElement.h"
#include "..\Scene Elements\Camera.h"
#include "..\Scene Elements\Light.h"
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

	uint IrrRenderer::GetIntesectionInfo(float x, float y, Vector3& dir, Vector3& inter)
	{
		if (!this->device || !this->device->run())
			return INVALID_ID;

		irr::scene::ISceneCollisionManager* collMan = this->smgr->getSceneCollisionManager();

		irr::core::vector2di coord((int)x, (int)y);
		irr::core::line3df ray = collMan->getRayFromScreenCoordinates(coord);
		dir = Vector3(ray.end.X, ray.end.Y, ray.end.Z) - Vector3(ray.start.X, ray.start.Y, ray.start.Z);
		dir.normalize();
		irr::core::vector3df intersection;
		irr::core::triangle3df hitTriangle;
		irr::scene::ISceneNode* hitSceneNode = collMan->getSceneNodeAndCollisionPointFromRay(ray, intersection, hitTriangle);
		if (hitSceneNode)
		{
			inter = Vector3(intersection.X, intersection.Y, intersection.Z);
			return hitSceneNode->getID();
		}
		inter = Vector3();
		return INVALID_ID;
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

		irr::scene::ICameraSceneNode* irrCamera = smgr->addCameraSceneNode();
		irrCamera->bindTargetAndRotation(true);
		irrCamera->setNearValue(0.1f);
		irrCamera->setFarValue(10000.0f);
		irrCamera->remove();

		return true;
	}


	void IrrRenderer::updateScene()
	{
		// Setup Camera
		Camera* camera = this->Owner->SceneManager->ActiveCamera;
		if (camera)
		{
			irr::scene::ICameraSceneNode* irrCamera = this->smgr->getActiveCamera();
			irrCamera->setFOV(camera->FOV * (3.14159265f / 180.0f)); // from deg to rad

			irrCamera->updateAbsolutePosition();
			const Vector3& pos = camera->Position;
			irrCamera->setPosition(irr::core::vector3df(pos.x, pos.y, pos.z));
			const Vector3& rot = camera->Rotation.toAxisAngle();
			irrCamera->setRotation(irr::core::vector3df(rot.x, rot.y, rot.z));
			const Vector3& scl = camera->Scale;
			irrCamera->setScale(irr::core::vector3df(scl.x, scl.y, scl.z));
		}

		// Set Global Ambient Light
		const Color4& ambientLight = this->Owner->SceneManager->AmbientLight;
		this->smgr->setAmbientLight(irr::video::SColorf(ambientLight.r, ambientLight.g, ambientLight.b, ambientLight.a));

		// TODO: fog
		// TODO: shadow

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

		
		irr::scene::ISceneNode* irrSceneNode = this->smgr->getSceneNodeFromId(sceneElement->ID);
		if (!irrSceneNode)
			irrSceneNode = this->createIrrSceneNode(sceneElement);

		irrSceneNode->setName(sceneElement->Name.c_str());
		if (sceneElement.get() == this->Owner->SceneManager->ActiveCamera) // hide active camera
			irrSceneNode->setVisible(false);
		else if (Engine::Mode != EngineMode::EEditor) // in Non-Editor mode
		{
			if (irrSceneNode->isDebugObject() == true) // hide all debug objects
				irrSceneNode->setVisible(false);
			else
				irrSceneNode->setVisible(sceneElement->Visible);
		}
		else 
			irrSceneNode->setVisible(true); // show all objects in Editor mode

		const Vector3& pos = sceneElement->Position;
		irrSceneNode->setPosition(irr::core::vector3df(pos.x, pos.y, pos.z));
		const Vector3& rot = sceneElement->Rotation.toAxisAngle();
		irrSceneNode->setRotation(irr::core::vector3df(rot.x, rot.y, rot.z));
		const Vector3& scl = sceneElement->Scale;
		irrSceneNode->setScale(irr::core::vector3df(scl.x, scl.y, scl.z));

		// TODO: set material

		irr::scene::ESCENE_NODE_TYPE type = irrSceneNode->getType();
		if (type == irr::scene::ESNT_MESH || type == irr::scene::ESNT_OCTREE)
		{
			irr::scene::IMeshSceneNode* irrMeshSceneNode = (irr::scene::IMeshSceneNode*) irrSceneNode;
			irr::scene::SMesh* irrMesh = (irr::scene::SMesh*)irrMeshSceneNode->getMesh();
			if (this->updateIrrMesh(sceneElement, irrMesh))
				irrMeshSceneNode->setMesh(irrMesh);
		}
		else if (type == irr::scene::ESNT_LIGHT && sceneElement->Type == SceneElementType::ELight)
		{
			Light* light = (Light*)sceneElement.get();

			irr::scene::ILightSceneNode* irrLightSceneNode = (irr::scene::ILightSceneNode*) irrSceneNode;
			irr::video::SLight irrLight = irrLightSceneNode->getLightData();
			irrLight.Radius = light->Radius;
			irrLight.AmbientColor = irr::video::SColorf(light->Color.r * 0.1f, light->Color.g * 0.1f, light->Color.b * 0.1f, light->Color.a * 0.1f);
			irrLight.DiffuseColor = irr::video::SColorf(light->Color.r, light->Color.g, light->Color.b, light->Color.a);
			irrLight.SpecularColor = irr::video::SColorf(light->Color.r / 8, light->Color.g / 8, light->Color.b / 8, light->Color.a / 8);
			irrLight.Falloff = light->SpotExponent;
			irrLight.InnerCone = light->SpotCutoffInner;
			irrLight.OuterCone = light->SpotCutoffOuter;
			irrLight.Attenuation = irr::core::vector3df(0, 0, 1.0f / light->Intensity);
			irrLightSceneNode->setLightType(irr::video::ELT_SPOT);
			irrLightSceneNode->setLightData(irrLight);
		}
	}

	irr::scene::ISceneNode* IrrRenderer::createIrrSceneNode(const SceneElementPtr sceneElement)
	{
		irr::scene::ISceneNode* irrSceneNode = NULL;
		irr::scene::ITriangleSelector* irrTriangleSelector = NULL;

		if (sceneElement->Type == SceneElementType::ELight)
		{
			irrSceneNode = this->smgr->addLightSceneNode();
			// TODO: add Billboard as a child when we have textures?
			// TODO: selector from the bounding box of the Billboard
			
			//irrTriangleSelector = this->smgr->createTriangleSelectorFromBoundingBox(irrSceneNode);
		}
		else
		{
			if (this->meshesCache.find(sceneElement->ContentID) == this->meshesCache.end())
				this->meshesCache[sceneElement->ContentID] = new irr::scene::SMesh();

			irr::scene::SMesh* irrMesh = this->meshesCache[sceneElement->ContentID];
			irrSceneNode = this->smgr->addOctreeSceneNode(irrMesh);
			if (sceneElement->Type == SceneElementType::ESystemObject)
				irrSceneNode->setIsDebugObject(true);

			// irrTriangleSelector = this->smgr->createOctreeTriangleSelector(mesh, irrSceneNode); // skip for some reason first object
			irrTriangleSelector = this->smgr->createTriangleSelectorFromBoundingBox(irrSceneNode);
		}

		// add triangle selector to be able to select it
		if (irrTriangleSelector)
		{
			irrSceneNode->setTriangleSelector(irrTriangleSelector);
			irrTriangleSelector->drop();
		}

		irrSceneNode->setID(sceneElement->ID);
		return irrSceneNode;
	}
	
	bool IrrRenderer::updateIrrMesh(const SceneElementPtr sceneElement, irr::scene::SMesh* irrMesh)
	{
		ContentElementPtr contentElement = this->Owner->ContentManager->GetElement(sceneElement->ContentID, true, true);
		if (!contentElement || contentElement->Type != ContentElementType::EMesh)
		{
			Engine::Log(EWarning, "GLRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid mesh (" +
				to_string(sceneElement->ContentID) + ")");
			// TODO: show somehow cube or something and show this only once
			return false;
		}
		Mesh* mesh = (Mesh*)contentElement.get();

		// Create irrMeshBuffer
		irr::scene::SMeshBuffer* irrMeshBuffer = NULL;
		if (irrMesh->getMeshBufferCount() != 0)
			irrMeshBuffer = (irr::scene::SMeshBuffer*)irrMesh->getMeshBuffer(0);
		else
		{
			irrMeshBuffer = new irr::scene::SMeshBuffer();
			irrMesh->addMeshBuffer(irrMeshBuffer);
			irrMeshBuffer->drop();
		}

		// Update irrMeshBuffer
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
			return true;
		}
		return false;
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

			if (Engine::Mode != EngineMode::EEngine)
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