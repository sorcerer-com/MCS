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
#include "..\Content Elements\Texture.h"


namespace MyEngine {

    irr::video::SColor IrrRenderer::irrInvalidColor = irr::video::SColor(255, 255, 0, 255);

    class IrrShaderCallBack : public irr::video::IShaderConstantSetCallBack
    {
    private:
        IrrRenderer* irrRenderer;
        int lightsCount;
        int textureSet;
        const int diffuseMapIdx = 0;
        const int normalMapIdx = 1;

    public:
        IrrShaderCallBack(IrrRenderer* irrRenderer)
        {
            this->irrRenderer = irrRenderer;
            this->lightsCount = 0;
            this->textureSet = 0;
        }

        IrrShaderCallBack& operator=(const IrrShaderCallBack&) { return *this; }

        virtual void OnSetMaterial(const irr::video::SMaterial& material)
        {
            if (material.Lighting)
            {
                irr::core::array<irr::scene::ISceneNode*> lights;
                this->irrRenderer->irrSmgr->getSceneNodesFromType(irr::scene::ESCENE_NODE_TYPE::ESNT_LIGHT, lights);
                for (int i = 0; i < (int)lights.size(); i++)
                {
                    if (!lights[i]->isVisible())
                    {
                        lights.erase(i);
                        i--;
                    }
                }
                this->lightsCount = lights.size();
            }
            else
                this->lightsCount = -1;

            this->textureSet = 0;
            for (int i = 0; i < irr::video::MATERIAL_MAX_TEXTURES; i++)
            {
                if (material.getTexture(i) != NULL)
                    this->textureSet |= 1 << i; // 2 pow i
            }
        }

        virtual void OnSetConstants(irr::video::IMaterialRendererServices* services, irr::s32)
        {
            services->setPixelShaderConstant("lightsCount", &this->lightsCount, 1);

            services->setPixelShaderConstant("textureSet", &this->textureSet, 1);
            
            services->setPixelShaderConstant("diffuseMap", &this->diffuseMapIdx, 1);
            services->setPixelShaderConstant("normalMap", &this->normalMapIdx, 1);
        }
    };
    

    IrrRenderer::IrrRenderer(Engine* owner) :
        Renderer(owner, EIrrRenderer)
    {
        this->windowHandle = NULL;

        this->irrDevice = NULL;
        this->irrDriver = NULL;
        this->irrSmgr = NULL;
        this->irrGuienv = NULL;

        this->irrMaterialType = irr::video::E_MATERIAL_TYPE::EMT_TRANSPARENT_ALPHA_CHANNEL;
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
        // Setup Irrlicht environment
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

        // Get managers
        this->irrDriver = this->irrDevice->getVideoDriver();
        this->irrSmgr = this->irrDevice->getSceneManager();
        this->irrGuienv = this->irrDevice->getGUIEnvironment();

        // Set options
        this->irrDriver->setTextureCreationFlag(irr::video::E_TEXTURE_CREATION_FLAG::ETCF_OPTIMIZED_FOR_QUALITY, true);
        this->irrDriver->setTextureCreationFlag(irr::video::E_TEXTURE_CREATION_FLAG::ETCF_CREATE_MIP_MAPS, true);

        this->irrSmgr->setShadowColor(irr::video::SColor(150, 0, 0, 0));
        this->irrSmgr->getParameters()->setAttribute(irr::scene::ALLOW_ZWRITE_ON_TRANSPARENT, true);

        // Set Shaders
        if (this->irrDriver->queryFeature(irr::video::E_VIDEO_DRIVER_FEATURE::EVDF_VERTEX_SHADER_1_1) &&
            this->irrDriver->queryFeature(irr::video::E_VIDEO_DRIVER_FEATURE::EVDF_ARB_VERTEX_PROGRAM_1) &&
            this->irrDriver->queryFeature(irr::video::E_VIDEO_DRIVER_FEATURE::EVDF_PIXEL_SHADER_1_1) &&
            this->irrDriver->queryFeature(irr::video::E_VIDEO_DRIVER_FEATURE::EVDF_ARB_FRAGMENT_PROGRAM_1))
        {
            irr::video::IGPUProgrammingServices* irrGPU = this->irrDriver->getGPUProgrammingServices();
            if (irrGPU)
            {
                IrrShaderCallBack* irrShaderCallback = new IrrShaderCallBack(this);
                this->irrMaterialType = irrGPU->addHighLevelShaderMaterialFromFiles(
                    "shader.vert", "main", irr::video::E_VERTEX_SHADER_TYPE::EVST_VS_1_1,
                    "shader.frag", "main", irr::video::E_PIXEL_SHADER_TYPE::EPST_PS_1_1,
                    irrShaderCallback, irr::video::E_MATERIAL_TYPE::EMT_TRANSPARENT_ALPHA_CHANNEL);
                // TODO: may be geometry shader
                irrShaderCallback->drop();
            }
        }

        // Setup irrCamera scene node
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
            irrCamera->setFOV(camera->FOV * (irr::core::PI / 180.0f)); // from deg to rad
            
            // Change camera's coordinate system from left-hand to right-hand
            irr::core::matrix4 projMatrix;
            projMatrix.buildProjectionMatrixPerspectiveFovRH(irrCamera->getFOV(), irrCamera->getAspectRatio(), irrCamera->getNearValue(), irrCamera->getFarValue());
            irrCamera->setProjectionMatrix(projMatrix);

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
        if (sceneElement->Type == SceneElementType::ESkyBox) irrSceneNode->setPosition(this->irrSmgr->getActiveCamera()->getPosition());
        const Vector3& rot = sceneElement->Rotation.toEulerAngle();
        irrSceneNode->setRotation(irr::core::vector3df(rot.x, rot.y, rot.z));
        const Vector3& scl = sceneElement->Scale;
        irrSceneNode->setScale(irr::core::vector3df(scl.x, scl.y, scl.z));
        irrSceneNode->updateAbsolutePosition();

        const auto irrChildrenSceneNode = irrSceneNode->getChildren();
        for (const auto& irrChildSceneNode : irrChildrenSceneNode)
        {
            irr::scene::ESCENE_NODE_TYPE type = irrChildSceneNode->getType();

            // Selection
            if (Engine::Mode == EngineMode::EEditor &&
                Selector::IsSelected(sceneElement->ID) &&
                type != irr::scene::ESCENE_NODE_TYPE::ESNT_LIGHT) // if scene element is selected
                irrChildSceneNode->setDebugDataVisible(irr::scene::E_DEBUG_SCENE_TYPE::EDS_BBOX);
            else
                irrChildSceneNode->setDebugDataVisible(irr::scene::E_DEBUG_SCENE_TYPE::EDS_OFF);

            if (type == irr::scene::ESCENE_NODE_TYPE::ESNT_MESH || type == irr::scene::ESCENE_NODE_TYPE::ESNT_OCTREE)
            {
                if (sceneElement->ContentID == INVALID_ID &&
                    sceneElement->Type == SceneElementType::ELight) // light elements is possible to haven't content
                {
                    irrChildSceneNode->setVisible(false);
                    continue;
                }
                else
                    irrChildSceneNode->setVisible(true);

                // Set Material
                irr::video::SMaterial& irrMaterial = irrChildSceneNode->getMaterial(0);
                this->updateIrrMaterial(sceneElement, irrMaterial);

                // Set Textures
                if (sceneElement->Textures.DiffuseMapID != INVALID_ID)
                {
                    ContentElementPtr contentElement = this->Owner->ContentManager->GetElement(sceneElement->Textures.DiffuseMapID, true, true);
                    if (contentElement && contentElement->Type == ContentElementType::ETexture)
                    {
                        irr::video::ITexture* irrTexture = this->irrDriver->getTexture(irr::core::stringw(to_string(sceneElement->Textures.DiffuseMapID).c_str()));
                        if (this->updateIrrTexture((Texture*)contentElement.get(), irrTexture) ||
                            irrMaterial.getTexture(0) != irrTexture)
                            irrMaterial.setTexture(0, irrTexture);
                    }
                }
                if (sceneElement->Textures.NormalMapID != INVALID_ID)
                {
                    ContentElementPtr contentElement = this->Owner->ContentManager->GetElement(sceneElement->Textures.NormalMapID, true, true);
                    if (contentElement && contentElement->Type == ContentElementType::ETexture)
                    {
                        irr::video::ITexture* irrTexture = this->irrDriver->getTexture(irr::core::stringw(to_string(sceneElement->Textures.NormalMapID).c_str()));
                        if (this->updateIrrTexture((Texture*)contentElement.get(), irrTexture) ||
                            irrMaterial.getTexture(1) != irrTexture)
                            irrMaterial.setTexture(1, irrTexture);
                    }
                }
                
                if (!sceneElement->Visible) // if scene element is invisible
                    irrMaterial.DiffuseColor.setAlpha(128);
                if (irrChildSceneNode->isDebugObject() || sceneElement->Type == SceneElementType::ELight || sceneElement->Type == SceneElementType::ESkyBox)
                    irrMaterial.Lighting = false;
                
                // Set Content
                if (this->meshesCache.find(sceneElement->ContentID) == this->meshesCache.end())
                    this->meshesCache[sceneElement->ContentID] = new irr::scene::SMesh();

                irr::scene::SMesh* irrMesh = this->meshesCache[sceneElement->ContentID];
                if (this->updateIrrMesh(sceneElement, irrMesh))
                {
                    irr::scene::IMeshSceneNode* irrMeshSceneNode = (irr::scene::IMeshSceneNode*) irrChildSceneNode;
                    irrMeshSceneNode->setMesh(irrMesh);
                    
                    irr::scene::ITriangleSelector* irrTriangleSelector = this->irrSmgr->createOctreeTriangleSelector(irrMesh, irrChildSceneNode);
                    irrChildSceneNode->setTriangleSelector(irrTriangleSelector);
                    irrTriangleSelector->drop();
                }

                if (irrChildSceneNode->getChildren().size() > 0) // update shadow
                {
                    irr::scene::IShadowVolumeSceneNode* irrShadowSceneNode = (irr::scene::IShadowVolumeSceneNode*)(*irrChildSceneNode->getChildren().begin());
                    if (irrShadowSceneNode)
                        irrShadowSceneNode->updateShadowVolumes();
                }
            }
            else if (type == irr::scene::ESCENE_NODE_TYPE::ESNT_LIGHT && sceneElement->Type == SceneElementType::ELight)
            {
                Light* light = (Light*)sceneElement.get();

                irr::scene::ILightSceneNode* irrLightSceneNode = (irr::scene::ILightSceneNode*) irrChildSceneNode;
                irrLightSceneNode->setRotation(irr::core::vector3df(90, 0, 0));
                irr::video::SLight irrLightData = irrLightSceneNode->getLightData();
                irrLightData.AmbientColor = irr::video::SColorf(light->Color.r * 0.1f, light->Color.g * 0.1f, light->Color.b * 0.1f, light->Color.a * 0.1f);
                irrLightData.DiffuseColor = irr::video::SColorf(light->Color.r, light->Color.g, light->Color.b, light->Color.a);
                irrLightData.SpecularColor = irr::video::SColorf(light->Color.r / 8, light->Color.g / 8, light->Color.b / 8, light->Color.a / 8);
                irrLightData.Falloff = light->SpotExponent;
                irrLightData.InnerCone = light->SpotCutoffInner;
                irrLightData.OuterCone = light->SpotCutoffOuter;
                irrLightData.Attenuation = irr::core::vector3df(0.0f, 1.0f / light->Radius, 1.0f / light->Intensity);
                irrLightSceneNode->setLightData(irrLightData);
                irrLightSceneNode->setLightType(irr::video::E_LIGHT_TYPE::ELT_SPOT);
                irrLightSceneNode->setVisible(light->Intensity != 0.0f); // if intensity is 0 light isn't visible
            }
            else if (type == irr::scene::ESCENE_NODE_TYPE::ESNT_BILLBOARD && sceneElement->Type == SceneElementType::ELight)
            {
                Light* light = (Light*)sceneElement.get();

                string lightTexture = light->Intensity != 0.0f ? "LightOn" : "LightOff";
                ContentElementPtr contentElement = this->Owner->ContentManager->GetElement("MPackage#Textures\\System\\" + lightTexture, true, true);
                if (contentElement && contentElement->Type == ContentElementType::ETexture)
                {
                    irr::scene::IBillboardSceneNode* irrBillboardSceneNode = (irr::scene::IBillboardSceneNode*) irrChildSceneNode;
                    irr::video::ITexture* irrTexture = this->irrDriver->getTexture(irr::core::stringw(to_string(contentElement->ID).c_str()));
                    irr::video::SMaterial& irrMaterial = irrBillboardSceneNode->getMaterial(0);
                    irrMaterial.MaterialType = irr::video::E_MATERIAL_TYPE::EMT_TRANSPARENT_ALPHA_CHANNEL;
                    irrMaterial.Lighting = false;
                    if (this->updateIrrTexture((Texture*)contentElement.get(), irrTexture) ||
                        irrMaterial.getTexture(0) != irrTexture)
                        irrMaterial.setTexture(0, irrTexture);

                    irrBillboardSceneNode->setScale(irr::core::vector3df(1.0f / scl.x, 1.0f / scl.y, 1.0f / scl.z)); // reset scale of the billbaord
                    if (Engine::Mode != EngineMode::EEditor) // in Non-Editor mode
                        irrBillboardSceneNode->setVisible(false);
                    else
                        irrBillboardSceneNode->setVisible(true);
                }
            }
            irrChildSceneNode->updateAbsolutePosition();
        }
    }


    irr::scene::ISceneNode* IrrRenderer::createIrrSceneNode(const SceneElementPtr sceneElement)
    {
        irr::scene::ISceneNode* irrSceneNode = this->irrSmgr->addEmptySceneNode(NULL, sceneElement->ID);

        if (sceneElement->Type == SceneElementType::ECamera ||
            sceneElement->Type == SceneElementType::ESystemObject)
            irrSceneNode->setIsDebugObject(true);

        if (this->meshesCache.find(sceneElement->ContentID) == this->meshesCache.end())
            this->meshesCache[sceneElement->ContentID] = new irr::scene::SMesh();

        irr::scene::SMesh* irrMesh = this->meshesCache[sceneElement->ContentID];
        //irr::scene::IMeshSceneNode* irrMeshSceneNode = this->irrSmgr->addOctreeSceneNode(irrMesh); // some objects disappears
        irr::scene::IMeshSceneNode* irrMeshSceneNode = this->irrSmgr->addMeshSceneNode(irrMesh, irrSceneNode, sceneElement->ID);

        // TODO: doesn't work with transparency
        // shadow
        //if (sceneElement->Type != SceneElementType::ELight &&
        //    sceneElement->Type != SceneElementType::ESkyBox)
        //    irrMeshSceneNode->addShadowVolumeSceneNode();

        // selector
        auto irrTriangleSelector = this->irrSmgr->createOctreeTriangleSelector(irrMesh, irrMeshSceneNode);
        irrMeshSceneNode->setTriangleSelector(irrTriangleSelector);
        irrTriangleSelector->drop();


        if (sceneElement->Type == SceneElementType::ELight)
        {
            irr::scene::ILightSceneNode* irrLightSceneNode = this->irrSmgr->addLightSceneNode(irrSceneNode);
            irrLightSceneNode->setID(sceneElement->ID);

            irr::scene::IBillboardSceneNode* irrBillboardSceneNode = this->irrSmgr->addBillboardSceneNode(irrSceneNode, irr::core::dimension2df(3.0f, 3.0f));
            irrBillboardSceneNode->setID(sceneElement->ID);
            irrBillboardSceneNode->setIsDebugObject(true);

            auto irrTriangleSelector = this->irrSmgr->createTriangleSelectorFromBoundingBox(irrBillboardSceneNode);
            irrBillboardSceneNode->setTriangleSelector(irrTriangleSelector);
            irrTriangleSelector->drop();
        }

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

            Engine::Log(LogType::EWarning, "IrrRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid mesh (" +
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
            irrMeshBuffer->setHardwareMappingHint(irr::scene::E_HARDWARE_MAPPING::EHM_STATIC, irr::scene::E_BUFFER_TYPE::EBT_VERTEX_AND_INDEX);
            irrMesh->setDirty();
            irrMesh->recalculateBoundingBox();
            return true;
        }
        return false;
    }

    bool IrrRenderer::updateIrrMaterial(const SceneElementPtr sceneElement, irr::video::SMaterial& irrMaterial)
    {
        if (sceneElement->MaterialID != INVALID_ID)
        {
            ContentElementPtr contentElement = NULL;
            if (this->Owner->ContentManager->ContainElement(sceneElement->MaterialID))
                contentElement = this->Owner->ContentManager->GetElement(sceneElement->MaterialID, true, true);
            if (!contentElement || contentElement->Type != ContentElementType::EMaterial)
            {
                if (irrMaterial.DiffuseColor == IrrRenderer::irrInvalidColor)
                    return false;

                Engine::Log(LogType::EWarning, "IrrRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid material (" +
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

            if (material->Textures.DiffuseMapID != INVALID_ID)
            {
                irr::video::ITexture* irrTexture = this->irrDriver->getTexture(irr::core::stringw(to_string(material->Textures.DiffuseMapID).c_str()));
                if (this->updateIrrTexture(material, material->Textures.DiffuseMapID, irrTexture) ||
                    irrMaterial.getTexture(0) != irrTexture)
                    irrMaterial.setTexture(0, irrTexture);
            }
            if (material->Textures.NormalMapID != INVALID_ID)
            {
                irr::video::ITexture* irrTexture = this->irrDriver->getTexture(irr::core::stringw(to_string(material->Textures.NormalMapID).c_str()));
                if (this->updateIrrTexture(material, material->Textures.NormalMapID, irrTexture) ||
                    irrMaterial.getTexture(1) != irrTexture)
                    irrMaterial.setTexture(1, irrTexture);
            }
        }

        irrMaterial.FogEnable = true;
        irrMaterial.BackfaceCulling = false;
        irrMaterial.NormalizeNormals = true;
        irrMaterial.Lighting = true;
        irrMaterial.ColorMaterial = irr::video::E_COLOR_MATERIAL::ECM_NONE;
        irrMaterial.MaterialType = (irr::video::E_MATERIAL_TYPE)irrMaterialType; // custom (shader) material
        return true;
    }

    bool IrrRenderer::updateIrrTexture(const Material* material, uint textureID, irr::video::ITexture*& irrTexture)
    {
        ContentElementPtr contentElement = NULL;
        if (this->Owner->ContentManager->ContainElement(textureID))
            contentElement = this->Owner->ContentManager->GetElement(textureID, true, true);
        if (!contentElement || contentElement->Type != ContentElementType::ETexture)
        {
            if (irrTexture != NULL)
                return false;

            string type = "";
            if (material->Textures.DiffuseMapID == textureID) type = "diffuse";
            else if (material->Textures.NormalMapID == textureID) type = "normal";
            Engine::Log(LogType::EWarning, "IrrRenderer", "Material '" + material->Name + "' (" + to_string(material->ID) + ") is referred to invalid " + type + " map (" +
                to_string(textureID) + ")");

            irrTexture = this->irrDriver->addTexture(irr::core::dimension2du(2, 2), irr::core::stringw(to_string(textureID).c_str()));
            return true;
        }
        Texture* texture = (Texture*)contentElement.get();

        return this->updateIrrTexture(texture, irrTexture);
    }

    bool IrrRenderer::updateIrrTexture(Texture* texture, irr::video::ITexture*& irrTexture)
    {
        if (irrTexture == NULL)
            irrTexture = this->irrDriver->addTexture(irr::core::dimension2du(texture->Width, texture->Height), irr::core::stringw(to_string(texture->ID).c_str()));

        if (irrTexture != NULL && texture->Changed)
        {
            byte* data = (byte*)irrTexture->lock(irr::video::E_TEXTURE_LOCK_MODE::ETLM_READ_WRITE);
            for (uint i = 0; i < texture->Width * texture->Height; i++)
            {
                // from RGBA to BGRA
                data[i * 4 + 2] = texture->Pixels[i * 4 + 0]; // R;
                data[i * 4 + 1] = texture->Pixels[i * 4 + 1]; // G;
                data[i * 4 + 0] = texture->Pixels[i * 4 + 2]; // B;
                data[i * 4 + 3] = texture->Pixels[i * 4 + 3]; // A;
            }
            irrTexture->unlock();
            irrTexture->regenerateMipMapLevels();
            texture->Changed = false;
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

        this->irrDriver->removeAllTextures();
        this->irrDriver->removeAllHardwareBuffers();
        this->irrDevice->closeDevice();
        this->irrDevice->drop();
    }

}