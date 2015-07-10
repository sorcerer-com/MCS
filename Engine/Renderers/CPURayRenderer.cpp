// CPURayRenderer.cpp

#include "stdafx.h"
#include "CPURayRenderer.h"

#pragma warning(push, 3)
namespace embree {
#include <Embree\rtcore.h>
#include <Embree\rtcore_geometry.h>
#include <Embree\rtcore_geometry_user.h>
#include <Embree\rtcore_ray.h>
#include <Embree\rtcore_scene.h>
}
#pragma warning(pop)

#include "..\Engine.h"
#include "..\Utils\Config.h"
#include "..\Utils\Types\Random.h"
#include "..\Utils\Types\Thread.h"
#include "..\Utils\Types\Profiler.h"
#include "..\Managers\SceneManager.h"
#include "..\Managers\ContentManager.h"
#include "..\Scene Elements\Camera.h"
#include "..\Scene Elements\Light.h"
#include "..\Content Elements\Mesh.h"
#include "..\Content Elements\Texture.h"


namespace MyEngine {

    pair<unsigned, Random> Random::rg_table[Random::RGENS];

    const int CPURayRenderer::VALID[RAYS] = { -1, -1, -1, -1 };

    
    CPURayRenderer::CPURayRenderer(Engine* owner) :
        ProductionRenderer(owner, RendererType::ECPURayRenderer)
    {
        this->RegionSize = 0;
        this->MinSamples = 0;
        this->MaxSamples = 0;
        this->SamplesThreshold = 0.0f;

        this->thread->defThreadPool();
        this->thread->defMutex("regions");

        this->phasePofiler = make_shared<Profiler>();
        
        this->rtcScene = NULL;
    }

    CPURayRenderer::~CPURayRenderer()
    {
        this->IsStarted = false;
        this->thread->joinWorkers();

        // clear scene
        if (this->rtcScene != NULL)
        {
            this->Regions.clear();
            this->contentElementCache.clear();

            this->rtcInstances.clear();
            for (const auto& rtcGeom : this->rtcGeometries)
                embree::rtcDeleteScene(rtcGeom.second);
            this->rtcGeometries.clear();
            embree::rtcDeleteScene(this->rtcScene);
            this->rtcScene = NULL;
            embree::rtcExit();
        }

        Engine::Log(LogType::ELog, "CPURayRenderer", "DeInit CPU Ray Renderer");
    }


    vector<string> CPURayRenderer::GetBufferNames()
    {
        return { "Diffuse", "Specular", "Reflection", "Refraction", "DirectLight", "IndirectLight", "TotalLight", "Samples", "Depth", "Final" };
    }
    
    vector<Region> CPURayRenderer::GetActiveRegions()
    {
        lock lck(this->thread->mutex("regions"));
        
        vector<Region> result;
        if (!this->IsStarted)
            return result;

        for (const auto& region : this->Regions)
        {
            if (region.active)
                result.push_back(region);
        }
        return result;
    }

    bool CPURayRenderer::Init(uint width, uint height)
    {
        ProfileLog;
        ProductionRenderer::Init(width, height);

        const auto& bufferNames = this->GetBufferNames();
        for (const auto& bufferName : bufferNames)
            this->Buffers[bufferName].init(width, height);

        Random::initRandom((int)Now);
        this->generateRegions();

        embree::rtcSetErrorFunction((embree::RTCErrorFunc)&onRTCError);
        
        Engine::Log(LogType::ELog, "CPURayRenderer", "Init CPU Ray Renderer to (" + to_string(width) + ", " + to_string(height) + ")");
        return true;
    }

    void CPURayRenderer::Start()
    {
        ProfileLog;
        ProductionRenderer::Start();

        // clear previous scene
        if (this->rtcScene != NULL)
        {
            this->contentElementCache.clear();

            this->rtcInstances.clear();
            for (const auto& rtcGeom : this->rtcGeometries)
                embree::rtcDeleteScene(rtcGeom.second);
            this->rtcGeometries.clear();
            embree::rtcDeleteScene(this->rtcScene);
            this->rtcScene = NULL;
            embree::rtcExit();
        }

        embree::rtcInit();
        this->beginFrame();
        this->createRTCScene();

        // preview phase
        this->phasePofiler->start();
        this->thread->addNTasks([&](int) { return this->render(true); }, (int)this->Regions.size());
        this->thread->addWaitTask();
        this->thread->addTask([&](int) { return this->sortRegions(); });
        this->thread->addWaitTask();
        this->thread->addTask([&](int) { Engine::Log(LogType::ELog, "CPURayRenderer", duration_to_string(this->phasePofiler->stop()) + " Preview phase time"); return true; });
        // render phase
        this->thread->addNTasks([&](int) { return this->render(false); }, (int)(this->Regions.size() + this->thread->workersCount() * 3 * 2));
        this->thread->addWaitTask();
        this->thread->addTask([&](int) { Engine::Log(LogType::ELog, "CPURayRenderer", duration_to_string(this->phasePofiler->stop()) + " Render phase time"); return true; });
        // post-processing phase
        this->thread->addTask([&](int) { return this->postProcessing(); });
        this->thread->addWaitTask();
        this->thread->addTask([&](int) { Engine::Log(LogType::ELog, "CPURayRenderer", duration_to_string(this->phasePofiler->stop()) + " Post-processing phase time"); return true; });
        this->thread->addTask([&](int) { this->Stop(); return true; });
        
        Engine::Log(LogType::ELog, "CPURayRenderer", "Start Rendering");
    }

    void CPURayRenderer::Stop()
    {
        ProductionRenderer::Stop();

        Engine::Log(LogType::ELog, "CPURayRenderer", "Stop Rendering");
    }


    void CPURayRenderer::generateRegions()
    {
        lock lck(this->thread->mutex("regions"));

        this->Regions.clear();
        int sw = (this->Width - 1) / this->RegionSize + 1;
        int sh = (this->Height -1) / this->RegionSize + 1;
        for (int y = 0; y < sh; y++)
        {
            for (int x = 0; x < sw; x++)
            {
                int left = x * this->RegionSize;
                int right = std::min(this->Width, (x + 1) * this->RegionSize);
                int top = y * this->RegionSize;
                int bottom = std::min(this->Height, (y + 1) * this->RegionSize);
                if (left == right || top == bottom)
                    continue;

                this->Regions.push_back(Region(left, top, right - left, bottom - top));
            }
        }
        this->nextRagion = 0;
    }

    bool CPURayRenderer::sortRegions()
    {
        lock lck(this->thread->mutex("regions"));

        if (!this->IsStarted)
            return false;

        // TODO: make somekind of anlize to determine delta
        sort(this->Regions.begin(), this->Regions.end(), [&](const Region& a, const Region& b) -> bool
        {
            const float delta = 10.0f;
            float aComplexity = (a.time * a.time) / 1000;
            float bComplexity = (b.time * b.time) / 1000;

            if (abs(aComplexity - bComplexity) > delta)
                return aComplexity < bComplexity;
            else
            {
                if (abs(a.y - b.y) > (int)this->RegionSize)
                    return a.y < b.y;
                else if (((a.y / this->RegionSize) / 2) % 2 == 0)
                    return a.x < b.x;
                else
                    return a.x > b.x;
            }
        });

        // split last 5 regions
        int numThreads = (int)this->thread->workersCount();
        vector<Region> temp;
        for (int i = 0; i < numThreads * 2; i++)
        {
            temp.push_back(this->Regions.back());
            this->Regions.pop_back();
        }
        reverse(temp.begin(), temp.end());
        for (const auto& region : temp)
        {
            this->Regions.push_back(Region(region.x, region.y, region.w / 2, region.h / 2));
            this->Regions.back().time = region.time * 0.25f;
            this->Regions.push_back(Region(region.x + region.w / 2, region.y, region.w / 2, region.h / 2));
            this->Regions.back().time = region.time * 0.25f;
            this->Regions.push_back(Region(region.x, region.y + region.h / 2, region.w / 2, region.h / 2));
            this->Regions.back().time = region.time * 0.25f;
            this->Regions.push_back(Region(region.x + region.w / 2, region.y + region.h / 2, region.w / 2, region.h / 2));
            this->Regions.back().time = region.time * 0.25f;
        }

        this->nextRagion = 0;
        return true;
    }

    void CPURayRenderer::beginFrame()
    {
        Camera* camera = this->Owner->SceneManager->ActiveCamera;

        upLeft = Vector3(-((float)this->Width / this->Height), 1.0f, 0.0f);
        float halfAngle = ((camera ? camera->FOV : 72.0f) / 2.0f) * PI / 180.0f;
        upLeft.normalize();
        upLeft *= (float)tan(halfAngle);
        upLeft.z = -1.0f;

        Vector3 upRight(upLeft);
        upRight.x = -upRight.x;
        Vector3 downLeft(upLeft);
        downLeft.y = -downLeft.y;

        up = Vector3(0.0f, 1.0f, 0.0f);
        right = Vector3(1.0f, 0.0f, 0.0f);
        front = Vector3(0.0f, 0.0f, -1.0f);

        pos = Vector3();
        focalPlaneDist = 0;
        fNumber = 2.0f;

        if (camera)
        {
            upLeft = camera->Rotation * upLeft;
            upRight = camera->Rotation * upRight;
            downLeft = camera->Rotation * downLeft;

            up = camera->Rotation * up;
            right = camera->Rotation * right;
            front = camera->Rotation * front;

            pos = camera->Position;
            focalPlaneDist = camera->FocalPlaneDist;
            fNumber = camera->FNumber;
        }

        dx = (upRight - upLeft) * (1.0f / this->Width);
        dy = (downLeft - upLeft) * (1.0f / this->Height);
    }

    embree::RTCRay CPURayRenderer::getRTCScreenRay(float x, float y) const
    {
        Vector3 start, dir;
        start = this->pos;
        dir = upLeft + dx * x + dy * y;
        dir.normalize();

        if (this->focalPlaneDist > 0.0f)
        {
            float cosTheta = dot(dir, front);
            float M = this->focalPlaneDist / cosTheta;
            Vector3 T = start + dir * M;

            Random& rand = Random::getRandomGen();
            float dx, dy;
            rand.unitDiscSample(dx, dy);

            dx *= 10.0f / this->fNumber;
            dy *= 10.0f / this->fNumber;

            start = start + dx * right + dy * up;
            dir = T - start;
            dir.normalize();
        }

        return RTCRay(start, dir);
    }
    

    void CPURayRenderer::createRTCScene()
    {
        // Create Scene
        embree::RTCSceneFlags sflags = embree::RTCSceneFlags::RTC_SCENE_STATIC | embree::RTCSceneFlags::RTC_SCENE_COHERENT;
        embree::RTCAlgorithmFlags aflags = embree::RTCAlgorithmFlags::RTC_INTERSECT1 | embree::RTCAlgorithmFlags::RTC_INTERSECT4;
        this->rtcScene = embree::rtcNewScene(sflags, aflags);

        // Create SceneElements
        vector<SceneElementPtr> sceneElements = this->Owner->SceneManager->GetElements();
        for (const auto& sceneElement : sceneElements)
        {
            if (sceneElement->ContentID == INVALID_ID || !sceneElement->Visible || 
                sceneElement->Type == SceneElementType::ECamera || sceneElement->Type == SceneElementType::ESystemObject)
                continue;

            embree::RTCScene rtcGeometry = this->createRTCGeometry(sceneElement);
            if (rtcGeometry != NULL)
            {
                uint rtcInstance = embree::rtcNewInstance(this->rtcScene, rtcGeometry);
                vector<float> matrix = getMatrix(sceneElement->Position, sceneElement->Rotation, sceneElement->Scale);
                embree::rtcSetTransform(this->rtcScene, rtcInstance, embree::RTCMatrixType::RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, &matrix[0]);
                embree::rtcUpdate(this->rtcScene, rtcInstance);

                this->rtcInstances[rtcInstance] = sceneElement;
            }

            this->cacheContentElements(sceneElement);
        }
        embree::rtcCommit(this->rtcScene);
    }

    embree::RTCScene CPURayRenderer::createRTCGeometry(const SceneElementPtr sceneElement)
    {
        if (this->rtcGeometries.find(sceneElement->ContentID) != this->rtcGeometries.end())
            return this->rtcGeometries[sceneElement->ContentID];

        // get mesh
        ContentElementPtr contentElement = NULL;
        if (this->Owner->ContentManager->ContainsElement(sceneElement->ContentID))
            contentElement = this->Owner->ContentManager->GetElement(sceneElement->ContentID, true, true);
        if (!contentElement || contentElement->Type != ContentElementType::EMesh)
        {
            Engine::Log(LogType::EWarning, "CPURayRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid mesh (" +
                to_string(sceneElement->ContentID) + ")");
            return NULL;
        }
        this->contentElementCache[sceneElement->ContentID] = contentElement;
        Mesh* mesh = (Mesh*)contentElement.get();

        // create rtcScene
        embree::RTCSceneFlags sflags = embree::RTCSceneFlags::RTC_SCENE_STATIC | embree::RTCSceneFlags::RTC_SCENE_COHERENT;
        embree::RTCAlgorithmFlags aflags = embree::RTCAlgorithmFlags::RTC_INTERSECT1 | embree::RTCAlgorithmFlags::RTC_INTERSECT4;
        embree::RTCScene rtcGeometry = embree::rtcNewScene(sflags, aflags);

        // create rtcMesh
        uint meshID = embree::rtcNewTriangleMesh(rtcGeometry, embree::RTCGeometryFlags::RTC_GEOMETRY_STATIC, mesh->Triangles.size(), mesh->Vertices.size());
        float* vertices = (float*)embree::rtcMapBuffer(rtcGeometry, meshID, embree::RTCBufferType::RTC_VERTEX_BUFFER);
        int* triangles = (int*)embree::rtcMapBuffer(rtcGeometry, meshID, embree::RTCBufferType::RTC_INDEX_BUFFER);
        for (int i = 0; i < mesh->Vertices.size(); i++)
        {
            vertices[i * 4 + 0] = mesh->Vertices[i].x;
            vertices[i * 4 + 1] = mesh->Vertices[i].y;
            vertices[i * 4 + 2] = mesh->Vertices[i].z;
        }
        for (int i = 0; i < mesh->Triangles.size(); i++)
        {
            triangles[i * 3 + 0] = mesh->Triangles[i].vertices[0];
            triangles[i * 3 + 1] = mesh->Triangles[i].vertices[1];
            triangles[i * 3 + 2] = mesh->Triangles[i].vertices[2];
        }
        embree::rtcUnmapBuffer(rtcGeometry, meshID, embree::RTCBufferType::RTC_VERTEX_BUFFER);
        embree::rtcUnmapBuffer(rtcGeometry, meshID, embree::RTCBufferType::RTC_INDEX_BUFFER);

        embree::rtcCommit(rtcGeometry);
        this->rtcGeometries[sceneElement->ContentID] = rtcGeometry;
        return rtcGeometry;
    }

    void CPURayRenderer::cacheContentElements(const SceneElementPtr sceneElement)
    {
        // scene element's material
        ContentElementPtr contentElement = NULL;
        if (this->contentElementCache.find(sceneElement->MaterialID) == this->contentElementCache.end())
        {
            if (this->Owner->ContentManager->ContainsElement(sceneElement->MaterialID))
                contentElement = this->Owner->ContentManager->GetElement(sceneElement->MaterialID, true, true);
            if (!contentElement || contentElement->Type != ContentElementType::EMaterial)
            {
                if (sceneElement->MaterialID != INVALID_ID)
                    Engine::Log(LogType::EWarning, "CPURayRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid material (" +
                        to_string(sceneElement->MaterialID) + ")");
                contentElement.reset();
            }
            this->contentElementCache[sceneElement->MaterialID] = contentElement;

            if (contentElement)
            {
                Material* material = (Material*)contentElement.get();
                // material's diffuse map texture
                contentElement = NULL;
                if (this->contentElementCache.find(material->Textures.DiffuseMapID) == this->contentElementCache.end())
                {
                    if (this->Owner->ContentManager->ContainsElement(material->Textures.DiffuseMapID))
                        contentElement = this->Owner->ContentManager->GetElement(material->Textures.DiffuseMapID, true, true);
                    if (!contentElement || contentElement->Type != ContentElementType::ETexture)
                    {
                        if (material->Textures.DiffuseMapID != INVALID_ID)
                            Engine::Log(LogType::EWarning, "CPURayRenderer", "Material '" + material->Name + "' (" + to_string(material->ID) + ") is referred to invalid texture (" +
                                to_string(material->Textures.DiffuseMapID) + ")");
                        contentElement.reset();
                    }
                    this->contentElementCache[material->Textures.DiffuseMapID] = contentElement;
                }
                // material's normal map texture
                contentElement = NULL;
                if (this->contentElementCache.find(material->Textures.NormalMapID) == this->contentElementCache.end())
                {
                    if (this->Owner->ContentManager->ContainsElement(material->Textures.NormalMapID))
                        contentElement = this->Owner->ContentManager->GetElement(material->Textures.NormalMapID, true, true);
                    if (!contentElement || contentElement->Type != ContentElementType::ETexture)
                    {
                        if (material->Textures.NormalMapID != INVALID_ID)
                            Engine::Log(LogType::EWarning, "CPURayRenderer", "Material '" + material->Name + "' (" + to_string(material->ID) + ") is referred to invalid texture (" +
                                to_string(material->Textures.NormalMapID) + ")");
                        contentElement.reset();
                    }
                    this->contentElementCache[material->Textures.NormalMapID] = contentElement;
                }
            }
        }

        // scene element's diffuse map texture
        contentElement = NULL;
        if (this->contentElementCache.find(sceneElement->Textures.DiffuseMapID) == this->contentElementCache.end())
        {
            if (this->Owner->ContentManager->ContainsElement(sceneElement->Textures.DiffuseMapID))
                contentElement = this->Owner->ContentManager->GetElement(sceneElement->Textures.DiffuseMapID, true, true);
            if (!contentElement || contentElement->Type != ContentElementType::ETexture)
            {
                if (sceneElement->Textures.DiffuseMapID != INVALID_ID)
                    Engine::Log(LogType::EWarning, "CPURayRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid texture (" +
                        to_string(sceneElement->Textures.DiffuseMapID) + ")");
                contentElement.reset();
            }
            this->contentElementCache[sceneElement->Textures.DiffuseMapID] = contentElement;
        }

        // scene element's normal map texture
        contentElement = NULL;
        if (this->contentElementCache.find(sceneElement->Textures.NormalMapID) == this->contentElementCache.end())
        {
            if (this->Owner->ContentManager->ContainsElement(sceneElement->Textures.NormalMapID))
                contentElement = this->Owner->ContentManager->GetElement(sceneElement->Textures.NormalMapID, true, true);
            if (!contentElement || contentElement->Type != ContentElementType::ETexture)
            {
                if (sceneElement->Textures.NormalMapID != INVALID_ID)
                    Engine::Log(LogType::EWarning, "CPURayRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid texture (" +
                        to_string(sceneElement->Textures.NormalMapID) + ")");
                contentElement.reset();
            }
            this->contentElementCache[sceneElement->Textures.NormalMapID] = contentElement;
        }
    }


    bool CPURayRenderer::render(bool preview)
    {
        const int delta = preview ? 4 : 1;
        if (this->nextRagion >= this->Regions.size())
            return false;

        // get region
        this->thread->mutex("regions").lock();
        Region& region = this->Regions[this->nextRagion];
        this->nextRagion++;
        this->thread->mutex("regions").unlock();

        Profiler prof;
        prof.start();
        region.active = preview ? false : true;

        // render
        for (int j = 0; j < region.h; j += delta)
        {
            for (int i = 0; i < region.w; i += delta)
            {
                if (!this->IsStarted)
                    return false;

                int x = region.x + i;
                int y = region.y + j;

                for (auto& buffer : this->Buffers)
                    buffer.second.setElement(x, y, Color4());

                uint samples = adaptiveSampling(this->MinSamples, this->MaxSamples, this->SamplesThreshold, [&](int) { return this->renderPixel(x, y); });

                float div = 1.0f / samples;
                for (auto& buffer : this->Buffers)
                    buffer.second.setElement(x, y, this->Buffers[buffer.first].getElement(x, y) * div);
                
                // TODO: add GI samples in Green chanel
                this->Buffers["Samples"].setElement(x, y, this->Buffers["Samples"].getElement(x, y) + Color4((float)(samples - 2) / (this->MaxSamples - 2), 0, 0));
                
                // do preview
                for (int p = 1; p < delta * delta; p++)
                {
                    uint xx = region.x + i + p % delta;
                    uint yy = region.y + j + p / delta;
                    for (auto& buffer : this->Buffers)
                        buffer.second.setElement(xx, yy, this->Buffers[buffer.first].getElement(x, y));
                }
            }

            if (!preview)
                this_thread::sleep_for(chrono::milliseconds(1));
        }

        region.active = false;
        region.time = (float)chrono::duration_cast<chrono::milliseconds>(prof.stop()).count();

        return true;
    }

    Color4 CPURayRenderer::renderPixel(int x, int y)
    {
        Profile;
        Random& rand = Random::getRandomGen();

        if (!this->IsStarted)
            return Color4();

        // intersect
        embree::RTCRay4 rtcRay4;
        for (int k = 0; k < RAYS; k++)
            setRTCRay4(rtcRay4, k, this->getRTCScreenRay(x + rand.randSample(RAYS / 2, k % 2), y + rand.randSample(RAYS / 2, k / 2)));
        embree::rtcIntersect4(VALID, this->rtcScene, rtcRay4);

        // compute color
        ColorsMapType colors;
        for (int k = 0; k < RAYS; k++)
        {
            if (rtcRay4.instID[k] == RTC_INVALID_GEOMETRY_ID)
                continue;

            const ColorsMapType& temp = this->computeColor(getRTCRay(rtcRay4, k));
            for (const auto& color : temp)
                colors[color.first] += color.second;
        }
        
        float div = 1.0f / RAYS;
        for (auto& color : colors)
            this->Buffers[color.first].setElement(x, y, this->Buffers[color.first].getElement(x, y) + color.second * div);
        
        return colors["Final"] * div;
    }

    using ColorsMapType = map < string, Color4 >; // buffer name / color
    ColorsMapType CPURayRenderer::computeColor(const embree::RTCRay& rtcRay)
    {
        Profile;
        ColorsMapType result;

        if (!this->IsStarted)
            return result;

        const SceneElementPtr& sceneElement = this->rtcInstances[rtcRay.instID];
        if (!sceneElement)
            return result;

        // proccess content
        Vector3 UV, normal;
        Mesh* mesh = (Mesh*)this->contentElementCache[sceneElement->ContentID].get();
        if (mesh)
        {
            const Triangle& triangle = mesh->Triangles[rtcRay.primID];
            const Vector3& tA = mesh->TexCoords[triangle.texCoords[0]];
            const Vector3& tB = mesh->TexCoords[triangle.texCoords[1]];
            const Vector3& tC = mesh->TexCoords[triangle.texCoords[2]];
            UV = barycentric(tA, tB, tC, rtcRay.u, rtcRay.v);

            const Vector3& nA = mesh->Normals[triangle.normals[0]];
            const Vector3& nB = mesh->Normals[triangle.normals[1]];
            const Vector3& nC = mesh->Normals[triangle.normals[2]];
            normal = barycentric(nA, nB, nC, rtcRay.u, rtcRay.v);
            normal = sceneElement->Rotation * normal;
            normal.normalize();
        }

        // process material
        Material* material = (Material*)this->contentElementCache[sceneElement->MaterialID].get();
        if (material)
        {
            result["Diffuse"] = material->DiffuseColor;
            Texture* diffuseMap = NULL;
            if (sceneElement->Textures.DiffuseMapID != INVALID_ID)
                diffuseMap = (Texture*)this->contentElementCache[sceneElement->Textures.DiffuseMapID].get();
            else
                diffuseMap = (Texture*)this->contentElementCache[material->Textures.DiffuseMapID].get();

            if (diffuseMap)
                result["Diffuse"] *= diffuseMap->GetColor(UV.x, UV.y);

            // TODO: normalmap
        }
        else if (sceneElement->Textures.DiffuseMapID != INVALID_ID)
        {
            Texture* diffuseMap = (Texture*)this->contentElementCache[sceneElement->Textures.DiffuseMapID].get();
            if (diffuseMap)
                result["Diffuse"] = diffuseMap->GetColor(UV.x, UV.y);
        }
        else
            result["Diffuse"] = Color4(1.0f, 1.0f, 1.0f, 1.0f);

        // calculate lighting
        if (sceneElement->Type == SceneElementType::EStaticObject)
        {
            const auto& lighting = this->getLighting(rtcRay, normal);
            for (const auto& color : lighting)
                result[color.first] = color.second;
            result["IndirectLight"] = this->Owner->SceneManager->AmbientLight; // TODO: add GI
        }
        else
            result["DirectLight"] = Color4(1.0f, 1.0f, 1.0f, 1.0f);

        // "Diffuse", "Specular", "Reflection", "Refraction", "DirectLight", "IndirectLight", "TotalLight", Samples, "Depth", "Final"
        result["TotalLight"] = result["DirectLight"] + result["IndirectLight"];

        float depth = rtcRay.tfar;
        result["Depth"] = Color4(depth, depth, depth);

        // TODO: add reflection / refraction
        float refract = 0.0f;
        float reflect = 0.0f;
        result["Final"] = result["Diffuse"] * result["TotalLight"] + result["Specular"];
        result["Final"] = result["Final"] * (1.0f - refract) + result["Refraction"] * refract;
        result["Final"] = result["Final"] * (1.0f - reflect) + result["Reflection"] * reflect;

        // fog
        float fogDensity = this->Owner->SceneManager->FogDensity;
        if (fogDensity > 0.0f)
        {
            float fogFactor = pow(2.0f, -fogDensity * fogDensity * rtcRay.tfar * rtcRay.tfar * LOG2);
            fogFactor = std::min(std::max(fogFactor, 0.0f), 1.0f);

            Color4 fog(1.0f, 1.0f, 1.0f, 1.0f);
            //if (!ray.getFlag(Ray::INDIRECT)) // TODO: only for primary rays
                fog += this->getFogLighting(rtcRay, fogFactor);

            result["Final"] = this->Owner->SceneManager->FogColor * fog * (1.0f - fogFactor) + result["Final"] * fogFactor;
        }

        return result;
    }

    ColorsMapType CPURayRenderer::getLighting(const embree::RTCRay& rtcRay, const Vector3& normal)
    {
        Profile;
        const uint MAXLIGHTS = 8;
        ColorsMapType lighting;

        Vector3 interPos = Vector3(rtcRay.org[0], rtcRay.org[1], rtcRay.org[2]) + Vector3(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]) * rtcRay.tfar; // TODO: many times calculated
        auto lights = this->Owner->SceneManager->GetElements(SceneElementType::ELight);
        if (lights.size() > MAXLIGHTS)
        {
            sort(lights.begin(), lights.end(), [&](const SceneElementPtr a, const SceneElementPtr b) -> bool
            {
                if (a->Visible != b->Visible)
                    return a->Visible > b->Visible;
                else
                {
                    Light* aLight = (Light*)a.get();
                    Light* bLight = (Light*)b.get();
                    float aContribution = aLight->Intensity / (aLight->Position - interPos).lengthSqr();
                    float bContribution = bLight->Intensity / (bLight->Position - interPos).lengthSqr();
                    return aContribution > bContribution;
                }
            });
        }

        int count = std::min((uint)lights.size(), MAXLIGHTS);
        for (int i = 0; i < count; i++)
        {
            if (!lights[i]->Visible)
                continue;

            ColorsMapType tempLighting;
            uint samples = adaptiveSampling(this->MinSamples, this->MaxSamples, this->SamplesThreshold, [&](int) -> Color4
            {
                const auto& temp = this->getLighting(rtcRay, (Light*)lights[i].get(), normal);
                for (const auto& color : temp)
                    tempLighting[color.first] += color.second;
                return temp.size() != 0 ? temp.at("DirectLight") : Color4();
            });

            float div = (1.0f / samples);
            lighting["DirectLight"] += tempLighting["DirectLight"] * div;
            lighting["Specular"] += tempLighting["Specular"] * div;
            lighting["Samples"] += Color4(0, 0, (float)(samples - 2) / (this->MaxSamples - 2));
        }

        lighting["DirectLight"].a = 1.0f;
        lighting["Specular"].a = 1.0f;
        lighting["Samples"].b /= count;
        return lighting;
    }

    ColorsMapType CPURayRenderer::getLighting(const embree::RTCRay& rtcRay, const Light* light, const Vector3& normal)
    {
        ColorsMapType lighting;
        lighting["DirectLight"] = Color4();
        lighting["Specular"] = Color4();
        
        Vector3 interPos = Vector3(rtcRay.org[0], rtcRay.org[1], rtcRay.org[2]) + Vector3(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]) * rtcRay.tfar;
        
        // light direction
        Vector3 lightDir = light->Rotation * Vector3(0.0f, -1.0f, 0.0f);
        lightDir.normalize();

        Vector3 shadowDirs[RAYS];
        embree::RTCRay4 rtcRay4;
        for (int i = 0; i < RAYS; i++)
        {
            Vector3 lightSample = this->getLightSample(light, RAYS, i);
            Vector3 dir = lightSample - interPos;
            shadowDirs[i] = dir;
            if (dir.length() > light->Radius * 1.10f)
                dir = Vector3();
            dir.normalize();

            setRTCRay4(rtcRay4, i, RTCRay(interPos, dir, 0.1f, shadowDirs[i].length() - 0.1f));
        }
        embree::rtcIntersect4(VALID, this->rtcScene, rtcRay4);

        for (int i = 0; i < RAYS; i++)
        {
            Vector3 shadowDir = shadowDirs[i];
            if (shadowDir.length() > light->Radius * 1.10f)
                continue;
            float lensq = shadowDir.lengthSqr();
            shadowDir.normalize();

            // test of occlusion
            // TODO: partial transparancy
            if (rtcRay4.instID[i] != RTC_INVALID_GEOMETRY_ID && this->rtcInstances[rtcRay4.instID[i]]->Type != SceneElementType::ELight)
                continue;

            // fog
            float fogFactor = 1.0f;
            if (this->Owner->SceneManager->FogDensity > 0.0f)
            {
                fogFactor = pow(2.0f, -this->Owner->SceneManager->FogDensity * this->Owner->SceneManager->FogDensity * lensq * LOG2);
                fogFactor = std::min(std::max(fogFactor, 0.0f), 1.0f);
                if (fogFactor == 0.0f)
                    continue;
            }

            // calculate lighting
            Color4 baseLight = light->Color * (light->Intensity / lensq);// *abs(dot(lightDir, -shadowDir)); // (cosine between the light's direction and the normed ray toward the hitpos)
            if (sqrt(lensq) > light->Radius)
                baseLight *= (light->Radius * 1.10f - sqrt(lensq)) / (light->Radius * 0.10f);

            // spot effect
            float spotEffect = dot(-shadowDir, lightDir);
            if (spotEffect < cos(light->SpotCutoffOuter * PI / 180.0f))
            {
                spotEffect = 0.0f;
                return lighting; // if spotEffect is 0 then no need for further calculations
            }
            else if (spotEffect > 0.0f)
                spotEffect = pow(spotEffect, light->SpotExponent);
            else
                spotEffect = 1.0f;

            float cosTheta = dot(shadowDir, normal);
            if (cosTheta > 0.0f)
                lighting["DirectLight"] += baseLight * cosTheta * spotEffect * fogFactor;

            // calculate specular
            Vector3 r = reflect(-shadowDir, normal);
            float cosGamma = dot(r, -Vector3(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]));
            if (cosGamma > 0.0f) // Specular
            {
                Material* mat = (Material*)this->contentElementCache[this->rtcInstances[rtcRay.instID]->MaterialID].get();
                if (mat)
                    lighting["Specular"] += baseLight * mat->SpecularColor * pow(cosGamma, 0.3f * mat->Shininess) * spotEffect * fogFactor;
            }
        }

        float div = 1.0f / RAYS;
        lighting["DirectLight"] *= div;
        lighting["Specular"] *= div;

        return lighting;
    }

    Vector3 CPURayRenderer::getLightSample(const Light* light, int numSamples, int sample)
    {
        Random& rand = Random::getRandomGen();
        Vector3 result;

        Mesh* mesh = (Mesh*)this->contentElementCache[light->ContentID].get();
        if (mesh && mesh->Name != "Cube")
        {
            int trglSample = (int)(rand.randSample(numSamples, sample) * (mesh->Triangles.size() - 1));
            Triangle& trgl = mesh->Triangles[trglSample];
            result = barycentric(mesh->Vertices[trgl.vertices[0]], mesh->Vertices[trgl.vertices[1]], mesh->Vertices[trgl.vertices[2]], rand.randSample(numSamples), rand.randSample(numSamples));
        }
        else
        {
            result = Vector3((rand.randSample(numSamples / 2, sample % 2) - 0.5f),
                             (rand.randSample(numSamples) - 0.5f),
                             (rand.randSample(numSamples / 2, sample / 2) - 0.5f)) * 20;
        }

        result *= light->Scale;
        result = light->Rotation * result;
        result += light->Position;
        return result;
    }

    Color4 CPURayRenderer::getFogLighting(const embree::RTCRay& rtcRay, float fogFactor)
    {
        Profile;
        Color4 result;

        Vector3 normal(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]);
        Color4 prevLighting;
        float delta = std::min(100.0f, rtcRay.tfar / 10);
        for (float dist = 0.0f; dist < rtcRay.tfar; dist += delta)
        {
            embree::RTCRay newRay = rtcRay;
            newRay.tfar = dist;
            Color4 lighting = this->getLighting(newRay, normal)["DirectLight"];
            
            float ration = lighting.intensity() / std::max(prevLighting.intensity(), 1.0f);
            if (dist > 0 && ration > 10.0f)
                dist -= delta;
            else
            {
                result += lighting * (delta / rtcRay.tfar);
                prevLighting = lighting;
            }

            if (dist > 0 && ration > 10.0f)
                delta = std::max(0.1f, delta * (0.5f * (1.0f - fogFactor)));

            if (dist > 0 && ration < 0.1f)
                delta = std::min(rtcRay.tfar / 10, delta * (2.0f / (1.0f - fogFactor)));
        }

        result.a = 1.0f;
        return result;
    }

    bool CPURayRenderer::postProcessing()
    {
        if (!this->IsStarted)
            return false;

        // normalize depth buffer
        float maxDepth = 0;
        for (uint j = 0; j < this->Height; j++)
        {
            for (uint i = 0; i < this->Width; i++)
            {
                maxDepth = std::max(maxDepth, this->Buffers["Depth"].getElement(i, j).r);
            }
        }
        for (uint j = 0; j < this->Height; j++)
        {
            for (uint i = 0; i < this->Width; i++)
            {
                Color4 c = this->Buffers["Depth"].getElement(i, j);
                c *= (1.0f / maxDepth);
                this->Buffers["Depth"].setElement(i, j, c);
            }
        }

        return true;
    }


    void CPURayRenderer::onRTCError(const embree::RTCError, const char* str)
    {
        Engine::Log(LogType::EError, "CPURayRenderer", "Embree: " +  string(str));
    }

    embree::RTCRay CPURayRenderer::RTCRay(const Vector3& start, const Vector3& dir, float near /* = 0.1f */, float far /* = 10000.0f */)
    {
        embree::RTCRay result;
        for (int i = 0; i < 3; i++)
        {
            result.org[i] = start[i];
            result.dir[i] = dir[i];
        }
        result.tnear = near;
        result.tfar = far;
        result.geomID = RTC_INVALID_GEOMETRY_ID;
        result.primID = RTC_INVALID_GEOMETRY_ID;
        result.instID = RTC_INVALID_GEOMETRY_ID;
        result.mask = -1;
        result.time = 0;
        return result;
    }

    void CPURayRenderer::setRTCRay4(embree::RTCRay4& ray_o, int i, const embree::RTCRay& ray_i)
    {
        ray_o.orgx[i] = ray_i.org[0];
        ray_o.orgy[i] = ray_i.org[1];
        ray_o.orgz[i] = ray_i.org[2];
        ray_o.dirx[i] = ray_i.dir[0];
        ray_o.diry[i] = ray_i.dir[1];
        ray_o.dirz[i] = ray_i.dir[2];
        ray_o.tnear[i] = ray_i.tnear;
        ray_o.tfar[i] = ray_i.tfar;
        ray_o.time[i] = ray_i.time;
        ray_o.mask[i] = ray_i.mask;

        ray_o.Ngx[i] = ray_i.Ng[0];
        ray_o.Ngy[i] = ray_i.Ng[1];
        ray_o.Ngz[i] = ray_i.Ng[2];
        ray_o.u[i] = ray_i.u;
        ray_o.v[i] = ray_i.v;
        ray_o.geomID[i] = ray_i.geomID;
        ray_o.primID[i] = ray_i.primID;
        ray_o.instID[i] = ray_i.instID;
    }

    embree::RTCRay CPURayRenderer::getRTCRay(const embree::RTCRay4& ray_i, int i)
    {
        embree::RTCRay ray_o;
        ray_o.org[0] = ray_i.orgx[i];
        ray_o.org[1] = ray_i.orgy[i];
        ray_o.org[2] = ray_i.orgz[i];
        ray_o.dir[0] = ray_i.dirx[i];
        ray_o.dir[1] = ray_i.diry[i];
        ray_o.dir[2] = ray_i.dirz[i];
        ray_o.tnear = ray_i.tnear[i];
        ray_o.tfar = ray_i.tfar[i];
        ray_o.time = ray_i.time[i];
        ray_o.mask = ray_i.mask[i];

        ray_o.Ng[0] = ray_i.Ngx[i];
        ray_o.Ng[1] = ray_i.Ngy[i];
        ray_o.Ng[2] = ray_i.Ngz[i];
        ray_o.u = ray_i.u[i];
        ray_o.v = ray_i.v[i];
        ray_o.geomID = ray_i.geomID[i];
        ray_o.primID = ray_i.primID[i];
        ray_o.instID = ray_i.instID[i];
        return ray_o;
    }

    uint CPURayRenderer::adaptiveSampling(uint minSamples, uint maxSamples, float samplesThreshold, const function<Color4(int)>& func)
    {
        Color4 color;
        uint sample = 0;
        for (sample = 1; sample <= maxSamples; sample++)
        {
            Color4 c = func(sample);

            if (sample > minSamples)
            {
                Color4 tempColor = color + c;
                Color4 c1 = color * (1.0f / (sample - 1));
                Color4 c2 = tempColor * (1.0f / sample);
                if (absolute(c1 - c2) < samplesThreshold)
                    break;
            }
            color += c;
        }
        return std::min(sample, maxSamples);
    }

}