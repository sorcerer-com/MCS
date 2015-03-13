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
#include "..\Content Elements\Material.h"


namespace MyEngine {

	irr::video::SColor IrrRenderer::irrInvalidColor = irr::video::SColor(255, 255, 0, 255);


	IrrRenderer::IrrRenderer(Engine* owner) :
		Renderer(owner, EIrrRenderer)
	{
		this->irrDevice = NULL;
		this->irrDriver = NULL;
		this->irrSmgr = NULL;
	}

	IrrRenderer::~IrrRenderer()
	{
		this->thread->join();

		Engine::Log(LogType::ELog, "IrrRenderer", "DeInit IrrLicht Renderer");
	}


	bool IrrRenderer::Init(void* params)
	{
		this->windowHandle = params;

		this->thread->defWorker(&IrrRenderer::render, this);

		Engine::Log(LogType::ELog, "IrrRenderer", "Init IrrLicht Renderer");
		return true;
	}

	void IrrRenderer::ReSize(int width, int height)
	{
		this->Width = width;
		this->Height = height;
		this->Resized = true;
		Engine::Log(LogType::ELog, "IrrRenderer", "IrrLicht renderer is resized to (" + to_string(width) + ", " + to_string(height) + ")");
	}

	uint IrrRenderer::GetIntesectionInfo(float x, float y, Vector3& dir, Vector3& inter)
	{
		if (!this->irrDevice || !this->irrDevice->run())
			return INVALID_ID;

		irr::scene::ISceneCollisionManager* collMan = this->irrSmgr->getSceneCollisionManager();

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
		irr::SIrrlichtCreationParameters irrParam;
		irrParam.AntiAlias = true;
		irrParam.DriverType = irr::video::E_DRIVER_TYPE::EDT_OPENGL;
		irrParam.HandleSRGB = true;
		irrParam.Stencilbuffer = true;
		irrParam.WindowId = this->windowHandle;
		irrParam.ZBufferBits = 32;

		this->irrDevice = irr::createDeviceEx(irrParam);
		if (!this->irrDevice)
		{
			Engine::Log(LogType::EError, "IrrRenderer", "Cannot init IrrLicht Renderer");
			return false;
		}

		this->irrDriver = this->irrDevice->getVideoDriver();
		this->irrSmgr = this->irrDevice->getSceneManager();
		this->irrGuienv = this->irrDevice->getGUIEnvironment();

		this->irrSmgr->setShadowColor(irr::video::SColor(150, 0, 0, 0));

		this->irrDriver->setTextureCreationFlag(irr::video::E_TEXTURE_CREATION_FLAG::ETCF_OPTIMIZED_FOR_QUALITY, true);
		this->irrDriver->setTextureCreationFlag(irr::video::E_TEXTURE_CREATION_FLAG::ETCF_CREATE_MIP_MAPS, true);

		irr::scene::ICameraSceneNode* irrCamera = this->irrSmgr->addCameraSceneNode();
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
			irr::scene::ICameraSceneNode* irrCamera = this->irrSmgr->getActiveCamera();
			irrCamera->setFOV(camera->FOV * (3.14159265f / 180.0f)); // from deg to rad

			irrCamera->updateAbsolutePosition();
			const Vector3& pos = camera->Position;
			irrCamera->setPosition(irr::core::vector3df(pos.x, pos.y, pos.z));
			const Vector3& rot = camera->Rotation.toEulerAngle();
			irrCamera->setRotation(irr::core::vector3df(rot.x, rot.y, rot.z));
			const Vector3& scl = camera->Scale;
			irrCamera->setScale(irr::core::vector3df(scl.x, scl.y, scl.z));
		}

		// Set Global Ambient Light
		const Color4& ambientLight = this->Owner->SceneManager->AmbientLight;
		this->irrSmgr->setAmbientLight(irr::video::SColorf(ambientLight.r, ambientLight.g, ambientLight.b, ambientLight.a));

		// Setup Fog
		const Color4& fogColor = this->Owner->SceneManager->FogColor;
		const float& fogDensity = this->Owner->SceneManager->FogDensity;
		this->irrDriver->setFog(irr::video::SColorf(fogColor.r, fogColor.g, fogColor.b, fogColor.a).toSColor(), irr::video::E_FOG_TYPE::EFT_FOG_EXP2, 50.0f, 100.0f, fogDensity, true, true);

		// Update SceneElements
		vector<SceneElementPtr> sceneElements = this->Owner->SceneManager->GetElements();
		for (const auto& sceneElement : sceneElements)
			this->updateSceneElement(sceneElement);
		sceneElements.clear();

		// Remove invalid irrSceneElements
		irr::scene::ISceneNode* irrRootSceneNode = this->irrSmgr->getRootSceneNode();
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
		irr::scene::ISceneNode* irrSceneNode = this->irrSmgr->getSceneNodeFromId(sceneElement->ID);
		if (!irrSceneNode)
			irrSceneNode = this->createIrrSceneNode(sceneElement);

		// Set Name and Visibility
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

		// Set Transformation
		const Vector3& pos = sceneElement->Position;
		irrSceneNode->setPosition(irr::core::vector3df(pos.x, pos.y, pos.z));
		const Vector3& rot = sceneElement->Rotation.toEulerAngle();
		irrSceneNode->setRotation(irr::core::vector3df(rot.x, rot.y, rot.z));
		const Vector3& scl = sceneElement->Scale;
		irrSceneNode->setScale(irr::core::vector3df(scl.x, scl.y, scl.z));

		// Set Material
		irr::video::SMaterial& irrMaterial = irrSceneNode->getMaterial(0);
		this->updateIrrMaterial(sceneElement, irrMaterial);

		// TODO: set textures

		// Set Content
		irr::scene::ESCENE_NODE_TYPE type = irrSceneNode->getType();
		if (type == irr::scene::ESCENE_NODE_TYPE::ESNT_MESH || type == irr::scene::ESCENE_NODE_TYPE::ESNT_OCTREE)
		{
			if (this->meshesCache.find(sceneElement->ContentID) == this->meshesCache.end())
				this->meshesCache[sceneElement->ContentID] = new irr::scene::SMesh();

			irr::scene::SMesh* irrMesh = this->meshesCache[sceneElement->ContentID];
			if (this->updateIrrMesh(sceneElement, irrMesh))
			{
				irr::scene::IMeshSceneNode* irrMeshSceneNode = (irr::scene::IMeshSceneNode*) irrSceneNode;
				irrMeshSceneNode->setMesh(irrMesh);
			}
		}
		else if (type == irr::scene::ESCENE_NODE_TYPE::ESNT_LIGHT && sceneElement->Type == SceneElementType::ELight)
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
			irrLightSceneNode->setLightType(irr::video::E_LIGHT_TYPE::ELT_SPOT);
			irrLightSceneNode->setLightData(irrLight);
		}
	}

	irr::scene::ISceneNode* IrrRenderer::createIrrSceneNode(const SceneElementPtr sceneElement)
	{
		irr::scene::ISceneNode* irrSceneNode = NULL;
		irr::scene::ITriangleSelector* irrTriangleSelector = NULL;

		if (sceneElement->Type == SceneElementType::ELight)
		{
			irrSceneNode = this->irrSmgr->addLightSceneNode();
			// TODO: add Billboard as a child when we have textures? set it as debug object
			// TODO: selector from the bounding box of the Billboard

			//irrTriangleSelector = this->irrSmgr->createTriangleSelectorFromBoundingBox(irrSceneNode);
		}
		else
		{
			if (this->meshesCache.find(sceneElement->ContentID) == this->meshesCache.end())
				this->meshesCache[sceneElement->ContentID] = new irr::scene::SMesh();

			irr::scene::SMesh* irrMesh = this->meshesCache[sceneElement->ContentID];
			irrSceneNode = this->irrSmgr->addOctreeSceneNode(irrMesh);
			if (sceneElement->Type == SceneElementType::ECamera ||
				sceneElement->Type == SceneElementType::ESystemObject)
				irrSceneNode->setIsDebugObject(true);

			// shadow
			irr::scene::IMeshSceneNode* irrMeshSceneNode = (irr::scene::IMeshSceneNode*) irrSceneNode;
			irrMeshSceneNode->addShadowVolumeSceneNode();

			// irrTriangleSelector = this->irrSmgr->createOctreeTriangleSelector(mesh, irrSceneNode); // skip for some reason first object
			irrTriangleSelector = this->irrSmgr->createTriangleSelectorFromBoundingBox(irrSceneNode);
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
		ContentElementPtr contentElement = NULL;
		if (this->Owner->ContentManager->ContainElement(sceneElement->ContentID))
			contentElement = this->Owner->ContentManager->GetElement(sceneElement->ContentID, true, true);
		if (!contentElement || contentElement->Type != ContentElementType::EMesh)
		{
			if (irrMesh->getMeshBufferCount() > 0 && irrMesh->getMeshBuffer(0)->getIndexCount() == 36)
				return false;

			Engine::Log(LogType::EWarning, "GLRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid mesh (" +
				to_string(sceneElement->ContentID) + ")");

			auto irrCubeSceneNode = this->irrSmgr->addCubeSceneNode();
			irrMesh->clear();
			irrMesh->addMeshBuffer(irrCubeSceneNode->getMesh()->getMeshBuffer(0));
			irrMesh->setDirty();
			irrCubeSceneNode->remove();
			return true;
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

	bool IrrRenderer::updateIrrMaterial(const SceneElementPtr sceneElement, irr::video::SMaterial& irrMaterial)
	{
		ContentElementPtr contentElement = NULL;
		if (this->Owner->ContentManager->ContainElement(sceneElement->MaterialID))
			contentElement = this->Owner->ContentManager->GetElement(sceneElement->MaterialID, true, true);
		if (!contentElement || contentElement->Type != ContentElementType::EMaterial)
		{
			if (irrMaterial.DiffuseColor == IrrRenderer::irrInvalidColor)
				return true;

			Engine::Log(LogType::EWarning, "GLRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid material (" +
				to_string(sceneElement->MaterialID) + ")");

			irrMaterial = irr::video::SMaterial();
			irrMaterial.DiffuseColor = IrrRenderer::irrInvalidColor;
			irrMaterial.ColorMaterial = irr::video::E_COLOR_MATERIAL::ECM_NONE;
			return true;
		}
		Material* material = (Material*)contentElement.get();

		irrMaterial.AmbientColor = irr::video::SColorf(material->AmbientColor.r, material->AmbientColor.g, material->AmbientColor.b, material->AmbientColor.a).toSColor();
		irrMaterial.DiffuseColor = irr::video::SColorf(material->DiffuseColor.r, material->DiffuseColor.g, material->DiffuseColor.b, material->DiffuseColor.a).toSColor();
		irrMaterial.SpecularColor = irr::video::SColorf(material->SpecularColor.r, material->SpecularColor.g, material->SpecularColor.b, material->SpecularColor.a).toSColor();
		irrMaterial.Shininess = material->Shininess;

		irrMaterial.FogEnable = true;
		irrMaterial.BackfaceCulling = false;
		irrMaterial.NormalizeNormals = true;
		irrMaterial.ColorMaterial = irr::video::E_COLOR_MATERIAL::ECM_NONE;
		return true;
	}


	void IrrRenderer::render()
	{
		if (!this->init())
			return;

		while (!this->thread->interrupted())
		{
			if (!this->irrDevice->run())
			{
				this_thread::sleep_for(chrono::milliseconds(10));
				continue;
			}

			if (this->Resized)
			{
				this->irrDriver->OnResize(irr::core::dimension2du(this->Width, this->Height));
				this->Resized = false;
			}

			this->updateScene();
			
			this->irrDriver->beginScene();

			this->irrSmgr->drawAll();
			this->irrGuienv->drawAll();

			if (Engine::Mode != EngineMode::EEngine)
			{
				irr::video::SColor white(255, 255, 255, 255);
				this->irrGuienv->getBuiltInFont()->draw(irr::core::stringw("FPS: ") + irr::core::stringw(this->irrDriver->getFPS()), irr::core::recti(10, 10, 0, 0), white);
				this->irrGuienv->getBuiltInFont()->draw(irr::core::stringw("Primitives: ") + irr::core::stringw(this->irrDriver->getPrimitiveCountDrawn()), irr::core::recti(10, 20, 0, 0), white);
			}

			this->irrDriver->endScene();

			this_thread::sleep_for(chrono::milliseconds(1));
		}

		this->irrDevice->closeDevice();
		this->irrDevice->drop();
	}

}