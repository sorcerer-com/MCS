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
#include "..\Scene Elements\RenderElement.h"
#include "..\Content Elements\Mesh.h"
#include "..\Content Elements\Texture.h"


namespace MyEngine {

    pair<unsigned, Random> Random::rg_table[Random::RGENS];
    function<int()> Random::threadIdFunc;

    const int CPURayRenderer::VALID[RAYS] = { -1, -1, -1, -1 };


    CPURayRenderer::CPURayRenderer(Engine* owner) :
        ProductionRenderer(owner, RendererType::ECPURayRenderer),
        lightCacheKdTree(3)
    {
        this->RegionSize = 64;
        this->VolumetricFog = true;
        this->MinSamples = 1;
        this->MaxSamples = 4;
        this->SampleThreshold = 0.01f;
        this->MaxLights = 8;
        this->MaxDepth = 4;
        this->GI = true;
        this->GISamples = 4;
        this->IrradianceMap = true;
        this->IrradianceMapSamples = 64;
        this->IrradianceMapDistanceThreshold = 0.5f;
        this->IrradianceMapNormalThreshold = 0.1f;
        this->IrradianceMapColorThreshold = 0.3f;
        this->LightCache = true;
        this->LightCacheSampleSize = 0.1f;

        this->thread->defThreadPool();
        this->thread->defMutex("regions");
        this->thread->defMutex("lights", mutex_type::read_write);
        this->thread->defMutex("lightCache", mutex_type::read_write);

        this->phasePofiler = make_shared<Profiler>();

        this->rtcScene = NULL;
        this->rtcIrrMapScene = NULL;
    }

    CPURayRenderer::~CPURayRenderer()
    {
        this->IsStarted = false;
        this->thread->joinWorkers();

        // clear scene
        if (this->rtcScene != NULL)
        {
            this->Regions.clear();
            this->contentElements.clear();

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
        return{ "Diffuse", "Specular", "DirectLight", "IndirectLight", "TotalLight", "Lighted", "Reflection", "Refraction", "Samples", "Depth", "Normals", "Final" };
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

    double CPURayRenderer::GetProgress()
    {
        lock lck(this->thread->mutex("regions"));

        if (!this->IsStarted)
            return 0.0;

        int finshed = 0;
        for (int i = 0; i < this->nextRagion; i++)
            finshed += this->Regions[i].w * this->Regions[i].h;

        return ((double)finshed / (this->Width * this->Height)) * 100;
    }

    bool CPURayRenderer::Init(uint width, uint height)
    {
        ProfileLog;
        ProductionRenderer::Init(width, height);

        const auto& bufferNames = this->GetBufferNames();
        for (const auto& bufferName : bufferNames)
            this->Buffers[bufferName].init(width, height);

        Random::initRandom((int)Now, []() -> int { return (int)this_thread::get_id().hash(); });
        this->generateRegions();

        embree::rtcSetErrorFunction((embree::RTCErrorFunc)&onRTCError);

        Engine::Log(LogType::ELog, "CPURayRenderer", "Init CPU Ray Renderer to (" + to_string(width) + ", " + to_string(height) + ")");
        return true;
    }

    void CPURayRenderer::Start()
    {
        ProfileLog;
        ProductionRenderer::Start();
        // TODO: progressive rendering
        // clear previous scene
        if (this->rtcScene != NULL)
        {
            this->lights.clear();
            this->contentElements.clear();
            this->irrMapSamples.clear();
            this->irrMapTriangles.clear();
            if (this->rtcIrrMapScene)
            {
                embree::rtcDeleteScene(this->rtcIrrMapScene);
                this->rtcIrrMapScene = NULL;
            }
            this->lightCacheSamples.clear();
            this->lightCacheKdTree.clear();

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
        // irradiance map phase
        if (this->GI && this->IrradianceMap)
        {
            this->thread->addTask([&](int) { return this->generateIrradianceMap(); });
            this->thread->addWaitTask();
            this->thread->addNTasks([&](int) { while (this->computeIrradianceMap()); return true; });
            this->thread->addWaitTask();
            this->thread->addTask([&](int) { Engine::Log(LogType::ELog, "CPURayRenderer", duration_to_string(this->phasePofiler->stop()) + " Irradiance Map phase time"); return true; });
        }
        // render phase
        this->thread->addNTasks([&](int) { return this->render(false); }, (int)(this->Regions.size() + this->thread->workersCount() * 3 * 2));
        this->thread->addWaitTask();
        this->thread->addTask([&](int) { Engine::Log(LogType::ELog, "CPURayRenderer", duration_to_string(this->phasePofiler->stop()) + " Render phase time"); return true; });
        // post-processing phase
        this->thread->addTask([&](int) { return this->postProcessing(); });
        this->thread->addWaitTask();
        this->thread->addTask([&](int) { Engine::Log(LogType::ELog, "CPURayRenderer", duration_to_string(this->phasePofiler->stop()) + " Post-processing phase time"); return true; });
        this->thread->addTask([&](int) { Engine::Log(LogType::ELog, "CPURayRenderer", to_string(this->lightCacheSamples.size()) + " light cache samples generated"); return true; });
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
        Profile;
        lock lck(this->thread->mutex("regions"));

        this->Regions.clear();
        const int sw = (this->Width - 1) / this->RegionSize + 1;
        const int sh = (this->Height - 1) / this->RegionSize + 1;
        for (int y = 0; y < sh; y++)
        {
            for (int x = 0; x < sw; x++)
            {
                int left = x * this->RegionSize;
                int right = min(this->Width, (x + 1) * this->RegionSize);
                int top = y * this->RegionSize;
                int bottom = min(this->Height, (y + 1) * this->RegionSize);
                if (left == right || top == bottom)
                    continue;

                this->Regions.push_back(Region(left, top, right - left, bottom - top));
            }
        }
        this->nextRagion = 0;
    }

    bool CPURayRenderer::sortRegions()
    {
        Profile;
        lock lck(this->thread->mutex("regions"));

        if (!this->IsStarted)
            return false;

        // add 50% time to region from its neighbors
        vector<float> times;
        const int sw = (this->Width - 1) / this->RegionSize + 1;
        for (int i = 0; i < (int)this->Regions.size(); i++)
        {
            float time = 0.0f;
            int count = 0;
            const int d = 3;
            for (int j = 0; j < d * d; j++)
            {
                int ii = i + (j % d - 1) + (j / d - 1) * sw; // i + xx + yy * sw
                if (ii > 0 && ii < (int)this->Regions.size() && ii != i)
                {
                    time += this->Regions[ii].time;
                    count++;
                }
            }
            time /= count;
            time *= 0.5f;

            time += this->Regions[i].time;
            times.push_back(time);
        }
        for (int i = 0; i < (int)this->Regions.size(); i++)
            this->Regions[i].time = times[i];

        // sort regions by complexity
        sort(this->Regions.begin(), this->Regions.end(), [&](const Region& a, const Region& b) -> bool
        {
            float aComplexity = (float)(a.time * a.time) / (a.w * a.h);
            float bComplexity = (float)(b.time * b.time) / (b.w * b.h);
            return aComplexity < bComplexity;
        });

        // start from region with smallest complexity then find closest to it, etc.
        vector<Region> temp = this->Regions;
        this->Regions.clear();
        while (!temp.empty())
        {
            Region region = temp[0];
            this->Regions.push_back(region);
            temp.erase(temp.begin());

            sort(temp.begin(), temp.end(), [&](const Region& a, const Region& b) -> bool
            {
                float aDist = (float)(a.x - region.x) * (a.x - region.x) + (a.y - region.y) * (a.y - region.y);
                float bDist = (float)(b.x - region.x) * (b.x - region.x) + (b.y - region.y) * (b.y - region.y);
                if (aDist != bDist)
                    return aDist < bDist;
                else
                {
                    float aComplexity = (float)(a.time * a.time) / (a.w * a.h);
                    float bComplexity = (float)(b.time * b.time) / (b.w * b.h);
                    return aComplexity < bComplexity;
                }
            });
        }

        // split last 5 regions
        int numThreads = (int)this->thread->workersCount();
        temp.clear();
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

        for (auto& region : this->Regions)
            region.time = 0.0f;

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
        }
        dir.normalize();

        return RTCRay(start, dir, 0);
    }


    void CPURayRenderer::createRTCScene()
    {
        Profile;

        // Create Scene
        embree::RTCSceneFlags sflags = embree::RTCSceneFlags::RTC_SCENE_STATIC | embree::RTCSceneFlags::RTC_SCENE_COHERENT;
        embree::RTCAlgorithmFlags aflags = embree::RTCAlgorithmFlags::RTC_INTERSECT1 | embree::RTCAlgorithmFlags::RTC_INTERSECT4;
        this->rtcScene = embree::rtcNewScene(sflags, aflags);

        // Create SceneElements
        vector<SceneElementPtr> sceneElements = this->Owner->SceneManager->GetElements();
        for (const auto& sceneElement : sceneElements)
        {
            if (sceneElement->ContentID == INVALID_ID || !sceneElement->Visible ||
                sceneElement->Type == SceneElementType::ECamera || sceneElement->Type == SceneElementType::ERenderObject)
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


        // Create System Scene
        this->rtcSystemScene = embree::rtcNewScene(sflags, aflags);

        // Create SceneElements
        sceneElements = this->Owner->SceneManager->GetElements(SceneElementType::ERenderObject);
        for (const auto& sceneElement : sceneElements)
        {
            if (sceneElement->ContentID == INVALID_ID || !sceneElement->Visible)
                continue;

            embree::RTCScene rtcGeometry = this->createRTCGeometry(sceneElement);
            if (rtcGeometry != NULL)
            {
                uint rtcInstance = embree::rtcNewInstance(this->rtcSystemScene, rtcGeometry);
                vector<float> matrix = getMatrix(sceneElement->Position, sceneElement->Rotation, sceneElement->Scale);
                embree::rtcSetTransform(this->rtcSystemScene, rtcInstance, embree::RTCMatrixType::RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, &matrix[0]);
                embree::rtcUpdate(this->rtcSystemScene, rtcInstance);

                this->rtcInstances[-(int)rtcInstance - 2] = sceneElement;
            }

            this->cacheContentElements(sceneElement);
        }
        embree::rtcCommit(this->rtcSystemScene);
    }

    embree::RTCScene CPURayRenderer::createRTCGeometry(const SceneElementPtr sceneElement)
    {
        if (this->rtcGeometries.find(sceneElement->ContentID) != this->rtcGeometries.end())
            return this->rtcGeometries[sceneElement->ContentID];

        // get mesh
        ContentElementPtr contentElement = NULL;
        if (this->Owner->ContentManager->ContainsElement(sceneElement->ContentID))
        {
            if (sceneElement->Type != SceneElementType::EDynamicObject)
                contentElement = this->Owner->ContentManager->GetElement(sceneElement->ContentID, true, true);
            else
                contentElement = this->Owner->ContentManager->GetInstance(sceneElement->ID, sceneElement->ContentID);
        }
        if (!contentElement || contentElement->Type != ContentElementType::EMesh)
        {
            Engine::Log(LogType::EWarning, "CPURayRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid mesh (" +
                to_string(sceneElement->ContentID) + ")");
            return NULL;
        }
        this->contentElements[sceneElement->ContentID] = contentElement;
        Mesh* mesh = (Mesh*)contentElement.get();

        // create rtcScene
        embree::RTCSceneFlags sflags = embree::RTCSceneFlags::RTC_SCENE_STATIC | embree::RTCSceneFlags::RTC_SCENE_COHERENT;
        embree::RTCAlgorithmFlags aflags = embree::RTCAlgorithmFlags::RTC_INTERSECT1 | embree::RTCAlgorithmFlags::RTC_INTERSECT4;
        embree::RTCScene rtcGeometry = embree::rtcNewScene(sflags, aflags);

        // create rtcMesh
        uint meshID = embree::rtcNewTriangleMesh(rtcGeometry, embree::RTCGeometryFlags::RTC_GEOMETRY_STATIC, mesh->Triangles.size(), mesh->Vertices.size());
        float* vertices = (float*)embree::rtcMapBuffer(rtcGeometry, meshID, embree::RTCBufferType::RTC_VERTEX_BUFFER);
        int* triangles = (int*)embree::rtcMapBuffer(rtcGeometry, meshID, embree::RTCBufferType::RTC_INDEX_BUFFER);
        for (int i = 0; i < (int)mesh->Vertices.size(); i++)
        {
            vertices[i * 4 + 0] = mesh->Vertices[i].x;
            vertices[i * 4 + 1] = mesh->Vertices[i].y;
            vertices[i * 4 + 2] = mesh->Vertices[i].z;
        }
        for (int i = 0; i < (int)mesh->Triangles.size(); i++)
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
        if (this->contentElements.find(sceneElement->MaterialID) == this->contentElements.end())
        {
            if (this->Owner->ContentManager->ContainsElement(sceneElement->MaterialID))
            {
                if (sceneElement->Type != SceneElementType::EDynamicObject)
                    contentElement = this->Owner->ContentManager->GetElement(sceneElement->MaterialID, true, true);
                else
                    contentElement = this->Owner->ContentManager->GetInstance(sceneElement->ID, sceneElement->MaterialID);
            }
            if (!contentElement || contentElement->Type != ContentElementType::EMaterial)
            {
                if (sceneElement->MaterialID != INVALID_ID)
                    Engine::Log(LogType::EWarning, "CPURayRenderer", "Scene element '" + sceneElement->Name + "' (" + to_string(sceneElement->ID) + ") is referred to invalid material (" +
                    to_string(sceneElement->MaterialID) + ")");
                contentElement.reset();
            }
            this->contentElements[sceneElement->MaterialID] = contentElement;

            if (contentElement)
            {
                Material* material = (Material*)contentElement.get();
                // material's diffuse map texture
                contentElement = NULL;
                if (this->contentElements.find(material->Textures.DiffuseMapID) == this->contentElements.end())
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
                    this->contentElements[material->Textures.DiffuseMapID] = contentElement;
                }
                // material's normal map texture
                contentElement = NULL;
                if (this->contentElements.find(material->Textures.NormalMapID) == this->contentElements.end())
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
                    this->contentElements[material->Textures.NormalMapID] = contentElement;
                }
            }
        }

        // scene element's diffuse map texture
        contentElement = NULL;
        if (this->contentElements.find(sceneElement->Textures.DiffuseMapID) == this->contentElements.end())
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
            this->contentElements[sceneElement->Textures.DiffuseMapID] = contentElement;
        }

        // scene element's normal map texture
        contentElement = NULL;
        if (this->contentElements.find(sceneElement->Textures.NormalMapID) == this->contentElements.end())
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
            this->contentElements[sceneElement->Textures.NormalMapID] = contentElement;
        }
    }

    InterInfo CPURayRenderer::getInterInfo(const embree::RTCRay& rtcRay, bool onlyColor /* = false */, bool noNormalMap /* = false */)
    {
        Profile;
        Vector3 rayDir(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]);

        InterInfo result;
        result.sceneElement = this->rtcInstances[rtcRay.instID];
        result.interPos = Vector3(rtcRay.org[0], rtcRay.org[1], rtcRay.org[2]) + rayDir * rtcRay.tfar;
        result.color = Color4::White();
        result.diffuse = 1.0f;
        result.refraction = 0.0f;
        result.reflection = 0.0f;

        if (result.sceneElement)
        {
            Mesh* mesh = (Mesh*)this->contentElements[result.sceneElement->ContentID].get();
            if (mesh)
            {
                const Triangle& triangle = mesh->Triangles[rtcRay.primID];
                const Vector3& tA = mesh->TexCoords[triangle.texCoords[0]];
                const Vector3& tB = mesh->TexCoords[triangle.texCoords[1]];
                const Vector3& tC = mesh->TexCoords[triangle.texCoords[2]];
                result.UV = barycentric(tA, tB, tC, rtcRay.u, rtcRay.v);

                if (!onlyColor)
                {
                    const Vector3& nA = mesh->Normals[triangle.normals[0]];
                    const Vector3& nB = mesh->Normals[triangle.normals[1]];
                    const Vector3& nC = mesh->Normals[triangle.normals[2]];
                    result.normal = barycentric(nA, nB, nC, rtcRay.u, rtcRay.v);
                    result.normal = result.sceneElement->Rotation * result.normal;
                }
            }

            Material* material = (Material*)this->contentElements[result.sceneElement->MaterialID].get();
            if (material)
            {
                result.color = material->DiffuseColor;
                result.refraction = 1.0f - material->DiffuseColor.a;
                result.reflection = 1.0f - material->SpecularColor.a;

                // diffuse map
                Texture* diffuseMap = NULL;
                if (result.sceneElement->Textures.DiffuseMapID != INVALID_ID)
                    diffuseMap = (Texture*)this->contentElements[result.sceneElement->Textures.DiffuseMapID].get();
                else
                    diffuseMap = (Texture*)this->contentElements[material->Textures.DiffuseMapID].get();

                if (diffuseMap)
                {
                    result.color *= diffuseMap->GetColor(result.UV.x, result.UV.y);
                    result.refraction = 1.0f - result.color.a;
                }

                // normal map
                if (!onlyColor)
                {
                    Texture* normalMap = NULL;
                    if (result.sceneElement->Textures.NormalMapID != INVALID_ID)
                        normalMap = (Texture*)this->contentElements[result.sceneElement->Textures.NormalMapID].get();
                    else
                        normalMap = (Texture*)this->contentElements[material->Textures.NormalMapID].get();

                    if (normalMap)
                    {
                        Color4 n = normalMap->GetColor(result.UV.x, result.UV.y);
                        if (!noNormalMap)
                        {
                            Vector3 bumpN = Vector3((n.r - 0.5f) * 2.0f, (n.g - 0.5f) * 2.0f, (n.b - 0.5f) * 2.0f);
                            Vector3 pn1, pn2;
                            orthonormedSystem(result.normal, pn1, pn2);
                            result.normal += pn1 * bumpN.x + pn2 * bumpN.y;
                        }

                        result.reflection = 1.0f - (material->SpecularColor.a * n.a);
                    }
                }
            }
            else // scene element's maps
            {
                // diffuse map
                Texture* diffuseMap = (Texture*)this->contentElements[result.sceneElement->Textures.DiffuseMapID].get();
                if (diffuseMap)
                {
                    result.color = diffuseMap->GetColor(result.UV.x, result.UV.y);
                    result.refraction = 1.0f - result.color.a;
                }

                // normal map
                if (!onlyColor)
                {
                    Texture* normalMap = (Texture*)this->contentElements[result.sceneElement->Textures.NormalMapID].get();
                    if (normalMap)
                    {
                        Color4 n = normalMap->GetColor(result.UV.x, result.UV.y);
                        Vector3 bumpN = Vector3((n.r - 0.5f) * 2.0f, (n.g - 0.5f) * 2.0f, (n.b - 0.5f) * 2.0f);
                        Vector3 pn1, pn2;
                        orthonormedSystem(result.normal, pn1, pn2);
                        result.normal += pn1 * bumpN.x + pn2 * bumpN.y;

                        result.reflection = 1.0f - n.a;
                    }
                }
            }
            result.normal.normalize();
            result.normal = faceforward(rayDir, result.normal);

            // fresnel
            if (result.reflection > 0.001f && result.reflection < 0.01f)
            {
                float ior = 1.0f / (material ? material->IOR : 1.5f);
                if (getFlag(rtcRay.align1, RayFlags::RAY_INSIDE))
                    ior = 1.0f / ior;
                result.reflection = fresnel(rayDir, result.normal, ior);
            }

            result.diffuse = (1.0f - result.refraction) * (1.0f - result.reflection);
            result.refraction *= (1.0f - result.reflection);

            // if total inner reflection do only reflection
            if (result.refraction > 0.01f)
            {
                float ior = 1.0f / (material ? material->IOR : 1.5f);
                if (getFlag(rtcRay.align1, RayFlags::RAY_INSIDE))
                    ior = 1.0f / ior;

                float NdotI = dot(rayDir, result.normal);
                float k = 1.0f - (ior * ior) * (1.0f - NdotI * NdotI);
                if (k < 0.0f)
                {
                    result.reflection += result.refraction;
                    result.refraction = 0.0f;
                }
            }
        }

        return result;
    }

    void CPURayRenderer::processRenderElements(embree::RTCRay& rtcRay, InterInfo& interInfo)
    {
        embree::RTCRay rtcSysRay = rtcRay;
        rtcSysRay.tfar = 10000.0f;
        rtcSysRay.geomID = RTC_INVALID_GEOMETRY_ID;
        rtcSysRay.primID = RTC_INVALID_GEOMETRY_ID;
        rtcSysRay.instID = RTC_INVALID_GEOMETRY_ID;

        embree::rtcIntersect(this->rtcSystemScene, rtcSysRay);
        rtcSysRay.instID = -rtcSysRay.instID - 2;
        const InterInfo& interInfoSys = this->getInterInfo(rtcSysRay);
        if (!interInfoSys.sceneElement || interInfoSys.sceneElement->Type != SceneElementType::ERenderObject)
            return;

        RenderElement* re = (RenderElement*)interInfoSys.sceneElement.get();
        if (re->RType == RenderElementType::ESlicer && rtcRay.tfar < rtcSysRay.tfar)
        {
            int count = 1;
            embree::RTCRay rtcPrevRay = rtcRay;
            while (rtcRay.tfar < rtcSysRay.tfar) // count intersections
            {
                rtcPrevRay = rtcRay;
                rtcRay.tnear = rtcRay.tfar + 0.01f;
                rtcRay.tfar = 10000.0f;
                rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
                rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
                rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
                embree::rtcIntersect(this->rtcScene, rtcRay);
                if (rtcRay.geomID == rtcPrevRay.geomID &&
                    rtcRay.instID == rtcPrevRay.instID)
                    count++;
                else
                    count = 1;
            }

            if (count % 2 == 0 && rtcRay.geomID == rtcPrevRay.geomID && rtcRay.instID == rtcPrevRay.instID) // if it's inside the object
            {
                rtcRay = rtcSysRay;

                interInfo = this->getInterInfo(rtcPrevRay);
                if (interInfoSys.sceneElement->MaterialID == INVALID_ID &&              // if slicer doesn't have material use the object's one
                    interInfoSys.sceneElement->Textures.DiffuseMapID == INVALID_ID &&
                    interInfoSys.sceneElement->Textures.NormalMapID == INVALID_ID)
                {
                    interInfo.interPos = interInfoSys.interPos;
                    interInfo.normal = interInfoSys.normal;
                }
                else
                    interInfo = interInfoSys;
            }
            else
                interInfo = this->getInterInfo(rtcRay);
        }
    }


    atomic_int nextSample;
    bool CPURayRenderer::generateIrradianceMap()
    {
        Profile;

        const int delta = this->RegionSize / 8;
        const float minDist = 1.0f;
        if (!this->IsStarted)
            return false;

        // generate initial samples
        Random& rand = Random::getRandomGen();
        const int w = (this->Width / delta) + 1;
        KdTree<Vector3> irrKdTree(2);
        for (uint j = 0; j <= this->Height; j += delta)
        {
            for (uint i = 0; i <= this->Width; i += delta)
            {
                if (!this->IsStarted)
                    return false;

                int x = min(i, this->Width - 1);
                if (x > 0 && x < (int)this->Width - 1) x += rand.randInt(0, delta / 2);
                int y = min(j, this->Height - 1);
                if (y > 0 && y < (int)this->Height - 1) y += rand.randInt(0, delta / 2);
                if (!this->addIrradianceMapSample(x + rand.randFloat(), y + rand.randFloat(), irrKdTree, minDist))
                    continue;

                if (j > 0 && i > 0) // create triangles
                {
                    int curr = (int)this->irrMapSamples.size() - 1;
                    bool d1 = (this->irrMapSamples[curr].id == this->irrMapSamples[curr - 1 - w].id); // the samples on diagonal 1 are on the same scene element
                    bool d2 = (this->irrMapSamples[curr - 1].id == this->irrMapSamples[curr - w].id); // the samples on diagonal 2 are on the same scene element
                    if ((d1 && !d2) || (d1 == d2 && (i / delta + j / delta) % 2 == 0))
                    {
                        this->addIrradianceMapTriangle(curr, curr - 1 - w, curr - 1);
                        this->addIrradianceMapTriangle(curr, curr - w, curr - 1 - w);
                    }
                    else
                    {
                        this->addIrradianceMapTriangle(curr, curr - w, curr - 1);
                        this->addIrradianceMapTriangle(curr - 1, curr - w, curr - 1 - w);
                    }
                }
            }
        }

        // generate more samples if it's needed
        for (int i = 0; i < (int)this->irrMapTriangles.size(); i += 3)
        {
            if (!this->IsStarted)
                return false;

            // spit between v1 and v2
            int v0 = -1, v1 = -1, v2 = -1, v3 = -1;

            // find edge to spilt - first by different scene elements, second on everthing else
            float dist = 0.0f;
            for (int j = 0; j < 3; j++)
            {
                const IrradianceMapSample& sample1 = this->irrMapSamples[this->irrMapTriangles[i + (j + 0) % 3]];
                const IrradianceMapSample& sample2 = this->irrMapSamples[this->irrMapTriangles[i + (j + 1) % 3]];
                if ((abs(sample1.x - sample2.x) < minDist * 2 && abs(sample1.y - sample2.y) < minDist * 2))
                    continue;
                if (sample1.color.intensity() < 0.0f && sample2.color.intensity() < 0.0f) // dont't split if there aren't static objects
                    continue;

                // if samples didn't good enought
                float d = abs(sample1.x - sample2.x) + abs(sample1.y - sample2.y);
                if ((sample1.id != sample2.id ||
                    (sample1.position - sample2.position).length() > this->IrradianceMapDistanceThreshold ||
                    (sample1.normal - sample2.normal).length() > this->IrradianceMapNormalThreshold ||
                    (sample1.color - sample2.color).intensity() > this->IrradianceMapColorThreshold) &&
                    d > dist)
                {
                    v1 = this->irrMapTriangles[i + (j + 0) % 3];
                    v2 = this->irrMapTriangles[i + (j + 1) % 3];
                    v0 = this->irrMapTriangles[i + (j + 2) % 3];
                    dist = d;
                }
            }

            if (v0 != -1 && v1 != -1 && v2 != -1)
            {
                // find opposite triangle
                int trgl2 = -1;
                for (int k = 0; k < (int)this->irrMapTriangles.size(); k += 3)
                {
                    int i1 = -1, i2 = -1, i3 = -1;
                    for (int j = 0; j < 3; j++)
                    {
                        if (this->irrMapTriangles[k + j] == v1)
                            i1 = k + j;
                        else if (this->irrMapTriangles[k + j] == v2)
                            i2 = k + j;
                        else if (this->irrMapTriangles[k + j] != v0)
                            i3 = k + j;
                    }
                    if (i1 != -1 && i2 != -1 && i3 != -1)
                    {
                        v3 = this->irrMapTriangles[i3];
                        trgl2 = k;
                        break;
                    }
                }

                // split
                if (v3 != -1)
                {
                    const IrradianceMapSample& sample1 = this->irrMapSamples[v1];
                    const IrradianceMapSample& sample2 = this->irrMapSamples[v2];
                    float x = (sample1.x + sample2.x) / 2.0f;
                    float y = (sample1.y + sample2.y) / 2.0f;
                    // if we split because of different scene elements find best split point
                    if (sample1.id != sample2.id)
                    {
                        for (float f = 0.1f; f < 1.0f; f += 0.1f)
                        {
                            x = sample1.x * (1.0f - f) + sample2.x * f;
                            y = sample1.y * (1.0f - f) + sample2.y * f;
                            if (abs(x - sample1.x) < minDist && abs(y - sample1.y) < minDist)
                                continue;
                            if (abs(x - sample2.x) < minDist && abs(y - sample2.y) < minDist)
                            {
                                f -= 0.1f;
                                x = sample1.x * (1.0f - f) + sample2.x * f;
                                y = sample1.y * (1.0f - f) + sample2.y * f;
                                break;
                            }
                            embree::RTCRay rtcRay = this->getRTCScreenRay(x, y);
                            embree::rtcIntersect(this->rtcScene, rtcRay);
                            if (this->rtcInstances[rtcRay.instID] && this->rtcInstances[rtcRay.instID]->ID == sample2.id)
                                break;
                        }
                    }

                    bool split = this->addIrradianceMapSample(x, y, irrKdTree, minDist);
                    if (split)
                    {
                        int curr = (int)this->irrMapSamples.size() - 1;
                        // split current triangle and opposite one also to 2 new triangles
                        this->addIrradianceMapTriangle(v0, v1, curr);
                        this->addIrradianceMapTriangle(v0, curr, v2);
                        this->addIrradianceMapTriangle(v3, curr, v1);
                        this->addIrradianceMapTriangle(v3, v2, curr);

                        // remove current and opposite triangle
                        if (trgl2 > i)
                        {
                            this->irrMapTriangles.erase(this->irrMapTriangles.begin() + trgl2, this->irrMapTriangles.begin() + trgl2 + 3);
                            this->irrMapTriangles.erase(this->irrMapTriangles.begin() + i, this->irrMapTriangles.begin() + i + 3);
                            i -= 3;
                        }
                        else
                        {
                            this->irrMapTriangles.erase(this->irrMapTriangles.begin() + i, this->irrMapTriangles.begin() + i + 3);
                            this->irrMapTriangles.erase(this->irrMapTriangles.begin() + trgl2, this->irrMapTriangles.begin() + trgl2 + 3);
                            i -= 6;
                        }
                    }
                    else if (abs(sample1.x - sample2.x) + abs(sample1.y - sample2.y) > abs(this->irrMapSamples[v0].x - this->irrMapSamples[v3].x) + abs(this->irrMapSamples[v0].y - this->irrMapSamples[v3].y))
                    {
                        // change the triangulation from (v0, v1, v2), (v3, v2, v1) to (v0, v1, v3), (v0, v3, v2)
                        this->irrMapTriangles[i + 0] = v0;
                        this->irrMapTriangles[i + 1] = v1;
                        this->irrMapTriangles[i + 2] = v3;
                        this->irrMapTriangles[trgl2 + 0] = v0;
                        this->irrMapTriangles[trgl2 + 1] = v3;
                        this->irrMapTriangles[trgl2 + 2] = v2;
                    }
                }
            }
        }

        /* Save to OBJ file
        ofstream ofile("1.obj");
        //for (int i = 0; i < this->irrMapSamples.size(); i++)
        //    ofile << "# " << i << endl  << "v " << (this->irrMapSamples[i].position.x - this->pos.x) << " " << (this->irrMapSamples[i].position.y - this->pos.y) << " " << (this->irrMapSamples[i].position.z - this->pos.z) << endl;
        // 2d samples
        for (int i = 0; i < this->irrMapSamples.size(); i++)
        ofile << "# " << i << endl << "v " << this->irrMapSamples[i].x << " " << -this->irrMapSamples[i].y << " " << -this->irrMapSamples[i].position.length() << endl;
        for (int i = 0; i < this->irrMapTriangles.size(); i += 3)
        ofile << "# " << i << endl << "f " << (this->irrMapTriangles[i + 0] + 1) << " " << (this->irrMapTriangles[i + 1] + 1) << " " << (this->irrMapTriangles[i + 2] + 1) << endl;
        ofile.close();//*/

        // create rtcScene
        embree::RTCSceneFlags sflags = embree::RTCSceneFlags::RTC_SCENE_STATIC | embree::RTCSceneFlags::RTC_SCENE_COHERENT;
        embree::RTCAlgorithmFlags aflags = embree::RTCAlgorithmFlags::RTC_INTERSECT1;
        this->rtcIrrMapScene = embree::rtcNewScene(sflags, aflags);

        // create rtcMesh
        uint meshID = embree::rtcNewTriangleMesh(this->rtcIrrMapScene, embree::RTCGeometryFlags::RTC_GEOMETRY_STATIC, this->irrMapTriangles.size() / 3, this->irrMapSamples.size());
        float* vertices = (float*)embree::rtcMapBuffer(this->rtcIrrMapScene, meshID, embree::RTCBufferType::RTC_VERTEX_BUFFER);
        int* triangles = (int*)embree::rtcMapBuffer(this->rtcIrrMapScene, meshID, embree::RTCBufferType::RTC_INDEX_BUFFER);
        for (int i = 0; i < (int)this->irrMapSamples.size(); i++)
        {
            vertices[i * 4 + 0] = this->irrMapSamples[i].position.x;
            vertices[i * 4 + 1] = this->irrMapSamples[i].position.y;
            vertices[i * 4 + 2] = this->irrMapSamples[i].position.z;
        }
        for (int i = 0; i < (int)this->irrMapTriangles.size(); i += 3)
        {
            triangles[i + 0] = this->irrMapTriangles[i + 0];
            triangles[i + 1] = this->irrMapTriangles[i + 1];
            triangles[i + 2] = this->irrMapTriangles[i + 2];
        }
        embree::rtcUnmapBuffer(this->rtcIrrMapScene, meshID, embree::RTCBufferType::RTC_VERTEX_BUFFER);
        embree::rtcUnmapBuffer(this->rtcIrrMapScene, meshID, embree::RTCBufferType::RTC_INDEX_BUFFER);

        embree::rtcCommit(this->rtcIrrMapScene);

        nextSample = 0;
        return true;
    }

    bool CPURayRenderer::addIrradianceMapSample(float x, float y, KdTree<Vector3>& irrKdTree, float minDist)
    {
        IrradianceMapSample newSample;
        newSample.x = x;
        newSample.y = y;

        // check if there is near sample already
        vector<int> indices;
        irrKdTree.find_nearest(Vector3(newSample.x, newSample.y, 0.0), [&](int idx) { return Vector3(this->irrMapSamples[idx].x, this->irrMapSamples[idx].y, 0.0); }, 1, indices);
        if (indices.size() > 0)
        {
            const IrradianceMapSample& sample = this->irrMapSamples[indices[0]];
            if (abs(newSample.x - sample.x) < minDist && abs(newSample.y - sample.y) < minDist)
                return false;
        }

        embree::RTCRay rtcRay = this->getRTCScreenRay(newSample.x, newSample.y);
        embree::rtcIntersect(this->rtcScene, rtcRay);
        newSample.color = Color4(-1.0f, -1.0f, -1.0f);
        if (rtcRay.instID != RTC_INVALID_GEOMETRY_ID)
        {
            InterInfo interInfo = this->getInterInfo(rtcRay, false, true);
            this->processRenderElements(rtcRay, interInfo);
            if (rtcRay.instID != RTC_INVALID_GEOMETRY_ID)
            {
                newSample.id = interInfo.sceneElement->ID;
                newSample.position = interInfo.interPos;
                newSample.normal = interInfo.normal;
                if (interInfo.sceneElement->Type == SceneElementType::EStaticObject)
                    newSample.color = this->getLighting(rtcRay, interInfo)["DirectLight"];
            }
        }
        this->irrMapSamples.push_back(newSample);

        irrKdTree.insert((int)this->irrMapSamples.size() - 1, [&](int idx) { return Vector3(this->irrMapSamples[idx].x, this->irrMapSamples[idx].y, 0.0); });
        this->Buffers["Samples"].setElement((uint)newSample.x, (uint)newSample.y, this->Buffers["Samples"].getElement((uint)newSample.x, (uint)newSample.y) + Color4(0.0f, 1.0f, 0.0f));
        return true;
    }

    void CPURayRenderer::addIrradianceMapTriangle(int v1, int v2, int v3)
    {
        this->irrMapTriangles.push_back(v1);
        this->irrMapTriangles.push_back(v2);
        this->irrMapTriangles.push_back(v3);
    }

    bool CPURayRenderer::computeIrradianceMap()
    {
        Profile;

        if (!this->IsStarted)
            return false;

        int sampleIdx = nextSample;
        nextSample++;
        if (sampleIdx >= (int)this->irrMapSamples.size())
        {
            this->nextRagion = 0;
            return false;
        }
        this->nextRagion = (int)(((float)nextSample / this->irrMapSamples.size()) * this->Regions.size());

        IrradianceMapSample& sample = this->irrMapSamples[sampleIdx];
        if (sample.color.intensity() < 0.0f) // if sample is nowhere or not static object
        {
            sample.color = Color4::Black();
            return true;
        }
        sample.color = Color4();
        uint samples = adaptiveSampling(this->IrradianceMapSamples, this->IrradianceMapSamples * 4, this->SampleThreshold, [&](int) -> Color4
        {
            const Vector3& dir = hemisphereSample(sample.normal);
            embree::RTCRay rtcGIRay = RTCRay(sample.position + sample.normal * 0.01f, dir, 1);
            setFlag(rtcGIRay.align1, RayFlags::RAY_INDIRECT, true);
            embree::rtcIntersect(this->rtcScene, rtcGIRay);

            InterInfo interInfoGI = this->getInterInfo(rtcGIRay);
            const Color4& lighting = this->getGILighting(rtcGIRay, interInfoGI, Color4::White());
            sample.color += lighting;
            return lighting;
        });
        sample.color *= (1.0f / samples);
        sample.color.a = 1.0f;
        this->Buffers["IndirectLight"].setElement((uint)sample.x, (uint)sample.y, sample.color);

        Color4 temp = this->Buffers["Samples"].getElement((uint)sample.x, (uint)sample.y);
        temp.g = (float)(samples - 2) / (this->IrradianceMapSamples * 4 - 2);
        this->Buffers["Samples"].setElement((uint)sample.x, (uint)sample.y, temp);

        return true;
    }


    bool CPURayRenderer::render(bool preview)
    {
        const int delta = preview ? this->RegionSize / 8 : 1;

        // get region
        this->thread->mutex("regions").lock();
        if (this->nextRagion >= (int)this->Regions.size())
        {
            this->thread->mutex("regions").unlock();
            return false;
        }
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

                uint minSamples = preview ? min(1u, this->MinSamples) : this->MinSamples;
                uint maxSamples = preview ? min(4u, this->MaxSamples) : this->MaxSamples;
                float sampleThreshold = preview ? min(0.01f, this->SampleThreshold) : this->SampleThreshold;
                uint samples = adaptiveSampling(minSamples, maxSamples, sampleThreshold, [&](int) { return this->renderPixel(x, y); });

                float div = 1.0f / samples;
                for (auto& buffer : this->Buffers)
                    buffer.second.setElement(x, y, this->Buffers[buffer.first].getElement(x, y) * div);

                this->Buffers["Samples"].setElement(x, y, this->Buffers["Samples"].getElement(x, y) + Color4((float)(samples - 2) / (maxSamples - 2), 0, 0));

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
                this_thread::sleep_for(chrono::milliseconds(10));
        }

        if (preview)
            this_thread::sleep_for(chrono::milliseconds(1));

        region.active = false;
        region.time = (float)chrono::duration_cast<chrono::milliseconds>(prof.stop()).count();

        return true;
    }

    Color4 CPURayRenderer::renderPixel(int x, int y)
    {
        Profile;
        Random& rand = Random::getRandomGen();

        if (!this->IsStarted)
            return Color4::Black();

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

            embree::RTCRay rtcRay = getRTCRay(rtcRay4, k);
            InterInfo interInfo = this->getInterInfo(rtcRay);
            this->processRenderElements(rtcRay, interInfo);
            const ColorsMapType& temp = this->computeColor(rtcRay, interInfo, 1.0f);
            for (const auto& color : temp)
                colors[color.first] += color.second;
        }

        float div = 1.0f / RAYS;
        for (auto& color : colors)
            this->Buffers[color.first].setElement(x, y, this->Buffers[color.first].getElement(x, y) + color.second * div);

        return colors["Final"] * div;
    }

    CPURayRenderer::ColorsMapType CPURayRenderer::computeColor(const embree::RTCRay& rtcRay, const InterInfo& interInfo, float contribution)
    {
        Profile;
        ColorsMapType result;

        if (!this->IsStarted)
            return result;

        if (!interInfo.sceneElement)
            return result;

        if (contribution < 0.02f)
            return result;

        result["Diffuse"] = interInfo.color * interInfo.diffuse;
        result["Diffuse"].a = 1.0f;

        // calculate lighting
        if ((interInfo.sceneElement->Type == SceneElementType::EStaticObject ||
             interInfo.sceneElement->Type == SceneElementType::EDynamicObject) &&
            interInfo.diffuse > 0.01f)
        {
            const auto& lighting = this->getLighting(rtcRay, interInfo);
            for (const auto& color : lighting)
                result[color.first] = color.second;

            // calculate GI
            bool bruteForce = true;
            if (this->GI && this->IrradianceMap && this->irrMapSamples.size() > 0 && rtcRay.align0 == 0 && 
                interInfo.sceneElement->Type != SceneElementType::EDynamicObject) // if irradiance map is enabled
            {
                result["IndirectLight"] = Color4::Black();

                embree::RTCRay rtcIrrRay = rtcRay;
                rtcIrrRay.tfar = 10000.0f;
                rtcIrrRay.geomID = RTC_INVALID_GEOMETRY_ID;
                rtcIrrRay.primID = RTC_INVALID_GEOMETRY_ID;
                rtcIrrRay.instID = RTC_INVALID_GEOMETRY_ID;
                embree::rtcIntersect(this->rtcIrrMapScene, rtcIrrRay);
                int triangle = rtcIrrRay.primID * 3;

                if (triangle >= 0)
                {
                    const IrradianceMapSample& sample1 = this->irrMapSamples[this->irrMapTriangles[triangle + 0]];
                    const IrradianceMapSample& sample2 = this->irrMapSamples[this->irrMapTriangles[triangle + 1]];
                    const IrradianceMapSample& sample3 = this->irrMapSamples[this->irrMapTriangles[triangle + 2]];
                    if (sample1.color.intensity() >= 0.0f && sample2.color.intensity() >= 0.0f && sample3.color.intensity() >= 0.0f)
                    {
                        result["IndirectLight"] = linearFilter(sample1.color, sample2.color, sample3.color, rtcIrrRay.u, rtcIrrRay.v);
                        bruteForce = false;
                    }
                }
            }
            if (bruteForce)
            {
                if (this->GI && (!this->IrradianceMap || (this->irrMapSamples.size() > 0 && rtcRay.align0 != 0) || 
                                 interInfo.sceneElement->Type == SceneElementType::EDynamicObject))
                {
                    uint giSamples = (uint)(this->GISamples * contribution * interInfo.diffuse);
                    giSamples = max(1u, giSamples);
                    uint samples = adaptiveSampling(giSamples, giSamples * 4, this->SampleThreshold, [&](int) -> Color4
                    {
                        const Vector3& dir = hemisphereSample(interInfo.normal);
                        embree::RTCRay rtcGIRay = RTCRay(interInfo.interPos + interInfo.normal * 0.01f, dir, (uint)rtcRay.align0 + 1);
                        rtcGIRay.align1 = rtcRay.align1; // flags
                        setFlag(rtcGIRay.align1, RayFlags::RAY_INDIRECT, true);
                        embree::rtcIntersect(this->rtcScene, rtcGIRay);

                        const InterInfo& interInfoGI = this->getInterInfo(rtcGIRay);
                        const Color4& lighting = this->getGILighting(rtcGIRay, interInfoGI, Color4::White());
                        result["IndirectLight"] += lighting;
                        return lighting;
                    });
                    result["IndirectLight"] *= (1.0f / samples);
                    result["Samples"] += Color4(0, (float)(samples - 2) / (this->GISamples * 4 - 2), 0);
                    result["Samples"].a = 1.0f;
                }
                else
                    result["IndirectLight"] = this->Owner->SceneManager->AmbientLight;
            }
        }
        else if (interInfo.sceneElement->Type != SceneElementType::EStaticObject &&
                 interInfo.sceneElement->Type != SceneElementType::EDynamicObject) // non static or dynamic objects
            result["DirectLight"] = Color4::White();

        result["TotalLight"] = (result["DirectLight"] + result["IndirectLight"]);
        result["TotalLight"].a = 1.0f;
        result["Lighted"] = result["Diffuse"] * result["TotalLight"] + result["Specular"];
        result["Lighted"].a = 1.0f;


        // calculate refraction
        if (interInfo.refraction > 0.01f && (uint)rtcRay.align0 < this->MaxDepth)
        {
            Material* material = (Material*)this->contentElements[interInfo.sceneElement->MaterialID].get();
            float glossiness = material ? material->Glossiness : 1.0f;

            Vector3 n = interInfo.normal;
            if (glossiness > 0.0f && glossiness < 0.999f) // glossy
                n = glossy(n, glossiness);

            float ior = 1.0f / (material ? material->IOR : 1.5f);
            if (getFlag(rtcRay.align1, RayFlags::RAY_INSIDE))
                ior = 1.0f / ior;
            Vector3 dir = refract(Vector3(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]), n, ior);

            if (dir.length() > 0.001f) // total inner reflection
            {
                embree::RTCRay rtcRefrRay = RTCRay(interInfo.interPos, dir, (uint)rtcRay.align0 + 1);
                rtcRefrRay.align1 = rtcRay.align1; // flags
                setFlag(rtcRefrRay.align1, RayFlags::RAY_INSIDE, !getFlag(rtcRay.align1, RayFlags::RAY_INSIDE));

                embree::rtcIntersect(this->rtcScene, rtcRefrRay);
                const InterInfo& interInfoRefr = this->getInterInfo(rtcRefrRay);

                result["Refraction"] = this->computeColor(rtcRefrRay, interInfoRefr, contribution * interInfo.refraction)["Final"];
            }
            result["Refraction"] *= interInfo.refraction;
            result["Refraction"].a = 1.0f;
        }
        else
            result["Refraction"] = Color4::Black();

        // calculate reflection
        if (interInfo.reflection > 0.01f && (uint)rtcRay.align0 < this->MaxDepth)
        {
            Material* material = (Material*)this->contentElements[interInfo.sceneElement->MaterialID].get();
            float glossiness = material ? material->Glossiness : 1.0f;

            Vector3 n = interInfo.normal;
            if (glossiness > 0.0f && glossiness < 0.999f) // glossy
                n = glossy(n, glossiness);

            Vector3 dir = reflect(Vector3(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]), n);
            embree::RTCRay rtcReflRay = RTCRay(interInfo.interPos, dir, (uint)rtcRay.align0 + 1);
            rtcReflRay.align1 = rtcRay.align1; // flags

            embree::rtcIntersect(this->rtcScene, rtcReflRay);
            const InterInfo& interInfoRefl = this->getInterInfo(rtcReflRay);

            result["Reflection"] = this->computeColor(rtcReflRay, interInfoRefl, contribution * interInfo.reflection)["Final"];
            result["Reflection"] *= interInfo.reflection;
            result["Reflection"].a = 1.0f;
        }
        else
            result["Reflection"] = Color4::Black();

        result["Final"] = result["Lighted"] + result["Refraction"] + result["Reflection"];
        result["Final"].a = 1.0f;

        float depth = rtcRay.tfar;
        result["Depth"] = Color4(depth, depth, depth);
        result["Normals"] = (Color4(interInfo.normal.x, interInfo.normal.y, interInfo.normal.z) + Color4(1.0f, 1.0f, 1.0f)) * 0.5f; // from range [-1:1] to [0:1]


        // fog
        float fogDensity = this->Owner->SceneManager->FogDensity;
        if (fogDensity > 0.0f)
        {
            float fogFactor = pow(2.0f, -fogDensity * fogDensity * rtcRay.tfar * rtcRay.tfar * LOG2);
            fogFactor = min(max(fogFactor, 0.0f), 1.0f);

            Color4 lighting(1.0f, 1.0f, 1.0f, 1.0f);
            if (this->VolumetricFog && !getFlag(rtcRay.align1, RayFlags::RAY_INDIRECT)) // volumetric fog only for primary rays
                lighting += this->getFogLighting(rtcRay);

            result["Final"] = this->Owner->SceneManager->FogColor * lighting * (1.0f - fogFactor) + result["Final"] * fogFactor;
        }

        // inside of the object (in computeColor, getLighting.shadow, getGILighting too)
        if (getFlag(rtcRay.align1, RayFlags::RAY_INSIDE))
        {
            Material* material = (Material*)this->contentElements[interInfo.sceneElement->MaterialID].get();
            if (material && material->InnerColor.intensity() > 0.01f)
            {
                float absFactor = exp(-rtcRay.tfar * material->Absorption);
                absFactor = min(max(absFactor, 0.0f), 1.0f);

                Color4 lighting(1.0f, 1.0f, 1.0f, 1.0f);
                if (!getFlag(rtcRay.align1, RayFlags::RAY_INDIRECT)) // volumetric fog only for primary rays
                    lighting += this->getFogLighting(rtcRay);
                result["Final"] = material->InnerColor * lighting * (1.0f - absFactor) + result["Final"] * absFactor;
            }
        }

        return result;
    }

    CPURayRenderer::ColorsMapType CPURayRenderer::getLighting(const embree::RTCRay& rtcRay, const InterInfo& interInfo)
    {
        Profile;
        ColorsMapType lighting;

        this->thread->rw_mutex("lights").read_lock();
        auto newLights = this->lights;
        this->thread->rw_mutex("lights").read_unlock();
        if (newLights.empty())
            newLights = this->Owner->SceneManager->GetElements(SceneElementType::ELight);
        uint count = this->MaxLights > 0 ? this->MaxLights : (uint)newLights.size();
        if (newLights.size() > count)
        {
            sort(newLights.begin(), newLights.end(), [&](const SceneElementPtr a, const SceneElementPtr b) -> bool
            {
                if (a->Visible != b->Visible)
                    return a->Visible > b->Visible;
                else
                {
                    Light* aLight = (Light*)a.get();
                    Light* bLight = (Light*)b.get();
                    if (aLight->LType != bLight->LType)
                        return aLight->LType < bLight->LType;

                    float aContribution = aLight->Color.intensity() * aLight->Intensity / (aLight->Position - interInfo.interPos).lengthSqr();
                    float bContribution = bLight->Color.intensity() * bLight->Intensity / (bLight->Position - interInfo.interPos).lengthSqr();
                    return aContribution > bContribution;
                }
            });
        }
        if (this->lights.empty())
        {
            this->thread->rw_mutex("lights").write_lock();
            this->lights = newLights;
            this->thread->rw_mutex("lights").write_unlock();
        }


        count = min((uint)newLights.size(), count);
        for (uint i = 0; i < count; i++)
        {
            // for indirect rays
            if (getFlag(rtcRay.align1, RayFlags::RAY_INDIRECT))
            {
                if (i > 0)
                    break;
                i = Random::getRandomGen().randInt(0, count - 1);
            }

            if (!newLights[i]->Visible)
                continue;

            ColorsMapType tempLighting;
            uint samples = adaptiveSampling(this->MinSamples, this->MaxSamples, this->SampleThreshold, [&](int) -> Color4
            {
                const auto& temp = this->getLighting(rtcRay, (Light*)newLights[i].get(), interInfo);
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
        lighting["Samples"].a = 1.0f;
        return lighting;
    }

    CPURayRenderer::ColorsMapType CPURayRenderer::getLighting(const embree::RTCRay& rtcRay, const Light* light, const InterInfo& interInfo)
    {
        ColorsMapType lighting;
        lighting["DirectLight"] = Color4::Black();
        lighting["Specular"] = Color4::Black();

        // light direction
        Vector3 lightDir = light->Rotation * Vector3(0.0f, -1.0f, 0.0f);
        lightDir.normalize();

        // calculate base lighting
        embree::RTCRay4 rtcRay4;
        Color4 baseLightings[RAYS];
        Vector3 shadowDirs[RAYS];
        float lightDists[RAYS];
        for (int i = 0; i < RAYS; i++)
        {
            lightDists[i] = 0.1f;
            setRTCRay4(rtcRay4, i, RTCRay(interInfo.interPos, Vector3(), 0, 0.1f, lightDists[i]));

            Vector3 lightSample = this->getLightSample(light, RAYS, i);
            shadowDirs[i] = lightSample - interInfo.interPos;
            if (shadowDirs[i].length() > light->Radius * 1.10f)
                continue;
            float lensq = max(shadowDirs[i].lengthSqr(), 1.0f);
            shadowDirs[i].normalize();

            // fog
            float fogFactor = 1.0f;
            if (this->Owner->SceneManager->FogDensity > 0.0f)
            {
                fogFactor = pow(2.0f, -this->Owner->SceneManager->FogDensity * this->Owner->SceneManager->FogDensity * lensq * LOG2);
                fogFactor = min(max(fogFactor, 0.0f), 1.0f);
                if (fogFactor == 0.0f)
                    continue;
            }

            // spot effect
            float spotEffect = dot(-shadowDirs[i], lightDir);
            if (spotEffect < cos(light->SpotCutoff * PI / 180.0f))
            {
                spotEffect = 0.0f;
                continue; // if spotEffect is 0 then no need for further calculations
            }
            else if (spotEffect > 0.0f)
                spotEffect = pow(spotEffect, light->SpotExponent);
            else
                spotEffect = 1.0f;

            // calculate lighting
            baseLightings[i] = light->Color * (light->Intensity / lensq);// *abs(dot(lightDir, -shadowDir)); // (cosine between the light's direction and the normed ray toward the hitpos)
            baseLightings[i] = baseLightings[i] * spotEffect * fogFactor;
            if (sqrt(lensq) > light->Radius)
                baseLightings[i] *= (light->Radius * 1.10f - sqrt(lensq)) / (light->Radius * 0.10f);

            lightDists[i] = sqrt(lensq) - 0.1f;
            setRTCRay4(rtcRay4, i, RTCRay(interInfo.interPos, shadowDirs[i], 0, 0.1f, lightDists[i]));
        }

        // shadow
        bool inside = getFlag(rtcRay.align1, RayFlags::RAY_INSIDE);
        while (lightDists[0] > 0.01f || lightDists[1] > 0.01f || lightDists[2] > 0.01f || lightDists[3] > 0.01f)
        {
            embree::rtcIntersect4(VALID, this->rtcScene, rtcRay4);

            for (int i = 0; i < RAYS; i++)
            {
                const InterInfo& shadowInterInfo = this->getInterInfo(getRTCRay(rtcRay4, i), true);
                if (rtcRay4.instID[i] != RTC_INVALID_GEOMETRY_ID && shadowInterInfo.sceneElement->Type != SceneElementType::ELight)
                {
                    baseLightings[i] *= shadowInterInfo.color * (1.0f - shadowInterInfo.color.a);

                    // inside of the object (in computeColor, getLighting.shadow, getGILighting too)
                    if (baseLightings[i].intensity() > 0.001f && inside)
                    {
                        Material* material = (Material*)this->contentElements[shadowInterInfo.sceneElement->MaterialID].get();
                        if (material && material->InnerColor.intensity() > 0.01f)
                        {
                            float absFactor = exp(-rtcRay.tfar * material->Absorption);
                            absFactor = min(max(absFactor, 0.0f), 1.0f);

                            baseLightings[i] = material->InnerColor * (1.0f - absFactor) + baseLightings[i] * absFactor;
                        }
                    }
                }

                // start = start + dir * dist
                rtcRay4.orgx[i] += rtcRay4.dirx[i] * rtcRay4.tfar[i];
                rtcRay4.orgy[i] += rtcRay4.diry[i] * rtcRay4.tfar[i];
                rtcRay4.orgz[i] += rtcRay4.dirz[i] * rtcRay4.tfar[i];
                if (baseLightings[i].intensity() < 0.001f)
                    lightDists[i] = 0.0f;
                else
                    lightDists[i] -= rtcRay4.tfar[i];
                rtcRay4.tfar[i] = lightDists[i];
            }
            inside = !inside;
        }

        // calculate lighting
        for (int i = 0; i < RAYS; i++)
        {
            // calculate diffuse
            float cosTheta = dot(shadowDirs[i], interInfo.normal);
            if (cosTheta > 0.0f)
                lighting["DirectLight"] += baseLightings[i] * cosTheta;

            // calculate specular
            Vector3 r = reflect(-shadowDirs[i], interInfo.normal);
            float cosGamma = dot(r, -Vector3(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]));
            if (cosGamma > 0.0f)
            {
                Material* mat = (Material*)this->contentElements[this->rtcInstances[rtcRay.instID]->MaterialID].get();
                if (mat)
                    lighting["Specular"] += baseLightings[i] * mat->SpecularColor * pow(cosGamma, 0.3f * mat->Shininess);
            }
        }

        float div = 1.0f / RAYS;
        lighting["DirectLight"] *= div;
        lighting["DirectLight"].a = 1.0f;
        lighting["Specular"] *= div;
        lighting["Specular"].a = 1.0f;

        return lighting;
    }

    Vector3 CPURayRenderer::getLightSample(const Light* light, int numSamples, int sample)
    {
        Random& rand = Random::getRandomGen();
        Vector3 result;

        Mesh* mesh = (Mesh*)this->contentElements[light->ContentID].get();
        if (mesh && mesh->Name != "Cube")
        {
            int trglSample = (int)(rand.randSample(numSamples, sample) * (mesh->Triangles.size() - 1));
            Triangle& trgl = mesh->Triangles[trglSample];
            result = barycentric(mesh->Vertices[trgl.vertices[0]], mesh->Vertices[trgl.vertices[1]], mesh->Vertices[trgl.vertices[2]], rand.randSample(numSamples), rand.randSample(numSamples));
        }
        else
        {
            int sqrtNumSamples = max((int)sqrt(numSamples), 1);
            result = Vector3((rand.randSample(sqrtNumSamples, sample % sqrtNumSamples) - 0.5f),
                (rand.randSample(numSamples) - 0.5f),
                (rand.randSample(sqrtNumSamples, sample / sqrtNumSamples) - 0.5f));
            result *= 20.0f;
        }

        result *= light->Scale;
        result = light->Rotation * result;
        result += light->Position;
        return result;
    }

    Color4 CPURayRenderer::getFogLighting(const embree::RTCRay& rtcRay)
    {
        Profile;
        const float deltaFactor = 1.01f;
        Color4 result;

        embree::RTCRay newRay = rtcRay;
        InterInfo interInfo;
        if (rtcRay.instID != RTC_INVALID_GEOMETRY_ID)
            interInfo.sceneElement = this->rtcInstances[rtcRay.instID];
        interInfo.normal = Vector3(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]);

        Color4 prevLighting;
        float delta = min(100.0f, rtcRay.tfar > 1000 ? rtcRay.tfar / 10 : rtcRay.tfar / 100);
        for (float dist = 0.0f; dist < rtcRay.tfar; dist += delta)
        {
            newRay.tfar = dist;
            interInfo.interPos = Vector3(newRay.org[0], newRay.org[1], newRay.org[2]) + Vector3(newRay.dir[0], newRay.dir[1], newRay.dir[2]) * newRay.tfar;
            Color4 lighting = this->getLighting(newRay, interInfo)["DirectLight"];

            float ration = lighting.intensity() / (prevLighting.intensity() == 0.0f ? 1.0f : prevLighting.intensity());
            if (dist > 0 && ration > 2.0f && delta > rtcRay.tfar / 1000)
            {
                dist -= delta;
                delta = max(rtcRay.tfar / 1000, delta / deltaFactor);
            }
            else
            {
                result += lighting * (delta / rtcRay.tfar);
                prevLighting = lighting;
            }

            if (dist > 0 && ration < 0.5f)
                delta = min(rtcRay.tfar / 10, delta * deltaFactor);
        }

        result.a = 1.0f;
        return result;
    }

    Color4 CPURayRenderer::getGILighting(const embree::RTCRay& rtcRay, const InterInfo& interInfo, const Color4& pathMultiplier)
    {
        Profile;
        Color4 result = Color4::Black();

        if ((uint)rtcRay.align0 >= this->MaxDepth || pathMultiplier.intensity() < 0.02f)
        {
            result = interInfo.color * this->Owner->SceneManager->AmbientLight;
            result.a = 1.0f;
            return result;
        }

        if (!this->IsStarted)
            return result;

        if (!interInfo.sceneElement)
            return result;

        // get from light cache
        if (this->LightCache && this->lightCacheSamples.size() > 0)
        {
            vector<int> indices;
            this->thread->rw_mutex("lightCache").read_lock();
            this->lightCacheKdTree.find_range(interInfo.interPos, [&](int idx) { return this->lightCacheSamples[idx].position; }, this->LightCacheSampleSize, indices);
            this->thread->rw_mutex("lightCache").read_unlock();
            if (indices.size() > 0) // get value
            {
                result = Color4();
                this->thread->rw_mutex("lightCache").read_lock();
                for (int idx : indices)
                    result += this->lightCacheSamples[idx].color;
                this->thread->rw_mutex("lightCache").read_unlock();
                result *= (1.0f / indices.size());
                return result;
            }
        }


        embree::RTCRay rtcNextRay;
        Color4 color;
        float pdf = 1.0f;

        Random& rand = Random::getRandomGen();
        float sample = rand.randFloat();

        // diffuse
        if ((interInfo.sceneElement->Type == SceneElementType::EStaticObject ||
             interInfo.sceneElement->Type == SceneElementType::EDynamicObject) &&
            sample <= interInfo.diffuse)
        {
            result = this->getLighting(rtcRay, interInfo)["DirectLight"] * pathMultiplier;
            result.a = 1.0f;

            // GI
            const Vector3& dir = hemisphereSample(interInfo.normal);
            rtcNextRay = RTCRay(interInfo.interPos + interInfo.normal * 0.01f, dir, (uint)rtcRay.align0 + 1);
            color = interInfo.color * (1.0f / PI) * max(0.0f, dot(interInfo.normal, dir));
            pdf = (1.0f / (2.0f * PI)) * interInfo.diffuse;
        }
        else if (sample <= interInfo.diffuse) // non static objects
        {
            result = interInfo.color;
            result.a = 1.0f;
            return result;
        }
        else
            result = Color4();

        // refraction
        if (interInfo.diffuse < sample && sample <= interInfo.diffuse + interInfo.refraction)
        {
            Material* material = (Material*)this->contentElements[interInfo.sceneElement->MaterialID].get();
            float glossiness = material ? material->Glossiness : 1.0f;

            Vector3 n = interInfo.normal;
            if (glossiness > 0.0f && glossiness < 0.999f) // glossy
                n = glossy(n, glossiness);

            float ior = 1.0f / (material ? material->IOR : 1.5f);
            if (getFlag(rtcRay.align1, RayFlags::RAY_INSIDE))
                ior = 1.0f / ior;
            Vector3 dir = refract(Vector3(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]), n, ior);

            if (dir.length() < 0.001f) // total inner reflection
                return Color4::Black();

            rtcNextRay = RTCRay(interInfo.interPos, dir, (uint)rtcRay.align0 + 1);
            setFlag(rtcNextRay.align1, RayFlags::RAY_INSIDE, !getFlag(rtcRay.align1, RayFlags::RAY_INSIDE));
            color = Color4::White();
            pdf = interInfo.refraction;
        }

        // reflection
        if (interInfo.diffuse + interInfo.refraction < sample)
        {
            Material* material = (Material*)this->contentElements[interInfo.sceneElement->MaterialID].get();
            float glossiness = material ? material->Glossiness : 1.0f;

            Vector3 n = interInfo.normal;
            if (glossiness > 0.0f && glossiness < 0.999f) // glossy
                n = glossy(n, glossiness);

            Vector3 dir = reflect(Vector3(rtcRay.dir[0], rtcRay.dir[1], rtcRay.dir[2]), n);
            rtcNextRay = RTCRay(interInfo.interPos, dir, (uint)rtcRay.align0 + 1);
            color = Color4::White();
            pdf = interInfo.reflection;
        }


        // trace
        rtcNextRay.align1 = rtcRay.align1;
        embree::rtcIntersect(this->rtcScene, rtcNextRay);
        const InterInfo& interInfoNext = this->getInterInfo(rtcNextRay);
        result += this->getGILighting(rtcNextRay, interInfoNext, pathMultiplier * color * (1.0f / pdf));
        result.a /= 2.0f;


        // fog
        float fogDensity = this->Owner->SceneManager->FogDensity;
        if (fogDensity > 0.0f)
        {
            float fogFactor = pow(2.0f, -fogDensity * fogDensity * rtcRay.tfar * rtcRay.tfar * LOG2);
            fogFactor = min(max(fogFactor, 0.0f), 1.0f);
            result = this->Owner->SceneManager->FogColor * (1.0f - fogFactor) + result * fogFactor;
        }

        // inside of the object (in computeColor, getLighting.shadow, getGILighting too)
        if (getFlag(rtcRay.align1, RayFlags::RAY_INSIDE))
        {
            Material* material = (Material*)this->contentElements[interInfo.sceneElement->MaterialID].get();
            if (material && material->InnerColor.intensity() > 0.01f)
            {
                float absFactor = exp(-rtcRay.tfar * material->Absorption);
                absFactor = min(max(absFactor, 0.0f), 1.0f);
                result = material->InnerColor * (1.0f - absFactor) + result * absFactor;
            }
        }


        result = interInfo.color * result;
        const int tries = 3;
        for (int i = 0; i < tries - 1; i++)
        {
            if (result.intensity() < 20.0f)
                break;
            embree::RTCRay rtcNextRay = rtcRay;
            rtcNextRay.align0++;
            result = this->getGILighting(rtcNextRay, interInfo, pathMultiplier);
        }

        // TODO: may be save LightCache and IrradianceMap to file?
        // set to light cache
        if (this->LightCache && interInfo.sceneElement->Type != SceneElementType::EDynamicObject)
        {
            // trace 2 more rays
            embree::RTCRay rtcNextRay = rtcRay;
            rtcNextRay.align0++;
            for (int i = 1; i < 3; i++)
                result += this->getGILighting(rtcNextRay, interInfo, pathMultiplier);
            result *= 1.0f / 3.0f;

            LightCacheSample sample;
            sample.position = interInfo.interPos;
            sample.color = result;
            this->thread->rw_mutex("lightCache").write_lock();
            this->lightCacheSamples.push_back(sample);
            this->lightCacheKdTree.insert((int)this->lightCacheSamples.size() - 1, [&](int idx) { return this->lightCacheSamples[idx].position; });
            this->thread->rw_mutex("lightCache").write_unlock();
        }

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
                maxDepth = max(maxDepth, this->Buffers["Depth"].getElement(i, j).r);
            }
        }
        for (uint j = 0; j < this->Height; j++)
        {
            for (uint i = 0; i < this->Width; i++)
            {
                Color4 c = this->Buffers["Depth"].getElement(i, j);
                c *= (1.0f / maxDepth);
                c.a = 1.0f;
                this->Buffers["Depth"].setElement(i, j, c);
            }
        }

        return true;
    }


    void CPURayRenderer::onRTCError(const embree::RTCError, const char* str)
    {
        Engine::Log(LogType::EError, "CPURayRenderer", "Embree: " + string(str));
    }

    embree::RTCRay CPURayRenderer::RTCRay(const Vector3& start, const Vector3& dir, uint depth, float near /* = 0.01f */, float far /* = 10000.0f */)
    {
        embree::RTCRay result;
        for (int i = 0; i < 3; i++)
        {
            result.org[i] = start[i];
            result.dir[i] = dir[i];
        }
        result.align0 = (float)depth; // depth
        result.align1 = 0; // flags
        result.align2 = 0;
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
        ray_o.align0 = 0;
        ray_o.dir[0] = ray_i.dirx[i];
        ray_o.dir[1] = ray_i.diry[i];
        ray_o.dir[2] = ray_i.dirz[i];
        ray_o.align1 = 0;
        ray_o.tnear = ray_i.tnear[i];
        ray_o.tfar = ray_i.tfar[i];
        ray_o.time = ray_i.time[i];
        ray_o.mask = ray_i.mask[i];

        ray_o.Ng[0] = ray_i.Ngx[i];
        ray_o.Ng[1] = ray_i.Ngy[i];
        ray_o.Ng[2] = ray_i.Ngz[i];
        ray_o.align2 = 0;
        ray_o.u = ray_i.u[i];
        ray_o.v = ray_i.v[i];
        ray_o.geomID = ray_i.geomID[i];
        ray_o.primID = ray_i.primID[i];
        ray_o.instID = ray_i.instID[i];
        return ray_o;
    }

    template <typename Func>
    uint CPURayRenderer::adaptiveSampling(uint minSamples, uint maxSamples, float sampleThreshold, const Func& func)
    {
        Color4 color;
        uint sample = 0;
        for (sample = 1; sample <= maxSamples; sample++)
        {
            Color4 c = func(sample);
            color += c;

            if (sample > minSamples)
            {
                Color4 c1 = (color - c) * (1.0f / (sample - 1));
                Color4 c2 = color * (1.0f / sample);
                if (absolute(c1 - c2) < sampleThreshold)
                    break;
            }
        }
        return min(sample, maxSamples);
    }

}