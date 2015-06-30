// IrrRenderer.cpp

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
#include "..\Content Elements\Mesh.h"


namespace MyEngine {

    pair<unsigned, Random> Random::rg_table[Random::RGENS];
    
    CPURayRenderer::CPURayRenderer(Engine* owner) :
        ProductionRenderer(owner, RendererType::ECPURayRenderer)
    {
        this->RegionSize = 0;

        this->thread->defMutex("regions");
    }

    CPURayRenderer::~CPURayRenderer()
    {
        this->thread->joinWorkers();
        Engine::Log(LogType::ELog, "CPURayRenderer", "DeInit CPU Ray Renderer");
    }


    vector<string> CPURayRenderer::GetBufferNames()
    {
        return { "Diffuse", "Specular", "Reflection", "Refraction", "DirectLight", "IndirectLight", "TotalLight", "Depth", "Final" };
    }
    
    vector<Region> CPURayRenderer::GetActiveRegions()
    {
        lock lck(this->thread->mutex("regions"));
        
        vector<Region> result;
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
        this->thread->joinWorkers();
        this->thread->defThreadPool();

        embree::rtcInit();
        this->beginFrame();
        this->createRTCScene();

        // preview phase
        this->thread->addNTasks([&](int) { return this->render(true); }, (int)this->Regions.size());
        this->thread->addWaitTask();
        this->thread->addTask([&](int) { return this->sortRegions(); });
        this->thread->addWaitTask();
        // render phase
        this->thread->addNTasks([&](int) { return this->render(false); }, (int)(this->Regions.size() + this->thread->workersCount() * 3 * 2));
        this->thread->addWaitTask();
        this->thread->addTask([&](int) { this->Stop(); return true; });
        
        Engine::Log(LogType::ELog, "CPURayRenderer", "Start Rendering");
    }

    void CPURayRenderer::Stop()
    {
        ProfileLog;
        this->rtcInstances.clear();
        for (const auto& rtcGeom : this->rtcGeometries)
            embree::rtcDeleteScene(rtcGeom.second);
        this->rtcGeometries.clear();
        embree::rtcDeleteScene(this->rtcScene);
        this->rtcScene = NULL;
        embree::rtcExit();

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

        if (camera)
        {
            upLeft = camera->Rotation * upLeft;
            upRight = camera->Rotation * upRight;
            downLeft = camera->Rotation * downLeft;

            up = camera->Rotation * up;
            right = camera->Rotation * right;
            front = camera->Rotation * front;
        }

        dx = (upRight - upLeft) * (1.0f / this->Width);
        dy = (downLeft - upLeft) * (1.0f / this->Height);
    }

    embree::RTCRay CPURayRenderer::getRTCScreenRay(float x, float y) const
    {
        Camera* camera = this->Owner->SceneManager->ActiveCamera;

        Vector3 start, dir;
        if (camera)
            start = camera->Position;
        dir = upLeft + dx * x + dy * y;
        dir.normalize();

        if (camera && camera->FocalPlaneDist > 0.0f)
        {
            float cosTheta = dot(dir, front);
            float M = camera->FocalPlaneDist / cosTheta;
            Vector3 T = start + dir * M;

            Random& rand = Random::getRandomGen();
            float dx, dy;
            rand.unitDiscSample(dx, dy);

            dx *= 10.0f / camera->FNumber; // TODO: get from fmiray is it ok?
            dy *= 10.0f / camera->FNumber;

            start = start + dx * right + dy * up;
            dir = T - start;
            dir.normalize();
        }

        embree::RTCRay result;
        for (int i = 0; i < 3; i++)
        {
            result.org[i] = start[i];
            result.dir[i] = dir[i];
        }
        result.tnear = 0.1f;
        result.tfar = 10000.0f;
        result.geomID = RTC_INVALID_GEOMETRY_ID;
        result.primID = RTC_INVALID_GEOMETRY_ID;
        result.instID = RTC_INVALID_GEOMETRY_ID;
        result.mask = -1;
        result.time = 0;
        return result;
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
            if (sceneElement->ContentID == INVALID_ID)
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


    bool CPURayRenderer::render(bool preview)
    {
        if (this->nextRagion >= this->Regions.size())
            return false;

        // get region
        this->thread->mutex("regions").lock();
        Region& region = this->Regions[this->nextRagion];
        this->nextRagion++;
        this->thread->mutex("regions").unlock();

        Profiler prof;
        prof.start();
        if (!preview) region.active = true;

        // rays
        vector<vector<embree::RTCRay4>> rays;
        int delta = preview ? 4 : 1;
        for (int j = 0; j < region.h; j += delta)
        {
            rays.push_back(vector<embree::RTCRay4>());
            for (int i = 0; i < region.w; i += delta * 4)
            {
                embree::RTCRay4 rtcRay4;
                for (int k = 0; k < 4; k++)
                    setRTCRay4(rtcRay4, k, this->getRTCScreenRay((float)region.x + i + k, (float)region.y + j));
                rays.rbegin()->push_back(rtcRay4);
            }
        }

        // intersect
        int valid4[4] = { -1, -1, -1, -1 };
        for (int j = 0; j < rays.size(); j++)
        {
            for (int i = 0; i < rays[j].size(); i++)
            {
                if (this->rtcScene == NULL)
                    return false;

                embree::rtcIntersect4(valid4, this->rtcScene, rays[j][i]);
                
                for (int k = 0; k < 4; k++)
                {
                    if (this->rtcScene == NULL)
                        return false;

                    int instID = rays[j][i].instID[k];
                    if (instID == RTC_INVALID_GEOMETRY_ID)
                        continue;
                    
                    const SceneElementPtr& elem = this->rtcInstances[instID];
                    if (!elem)
                        continue;
                    Material* mat = (Material*)elem->GetMaterial().get();
                    Color4 color;
                    if (mat != NULL)
                        color = mat->DiffuseColor;
                    
                    float depth = rays[j][i].tfar[k] / 1000.0f;
                    for (int p = 0; p < delta * delta; p++)
                    {
                        uint x = region.x + i * delta * 4 + k * delta + p % delta;
                        uint y = region.y + j * delta + p / delta;
                        this->Buffers["Final"].setElement(x, y, color);
                        this->Buffers["Depth"].setElement(x, y, Color4(depth, depth, depth));
                    }
                }
            }
        }

        if (!preview) region.active = false;
        region.time = (float)chrono::duration_cast<chrono::milliseconds>(prof.stop()).count();

        return true;
    }

    bool CPURayRenderer::sortRegions()
    {
        lock lck(this->thread->mutex("regions"));

        sort(this->Regions.begin(), this->Regions.end(), [](const Region& a, const Region& b) -> bool
        {
            if (fabs(a.time - b.time) >= 1.0f)
                return a.time < b.time;
            else
            {
                if (a.y != b.y)
                    return a.y < b.y;
                else
                    return a.x < b.x;
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


    void CPURayRenderer::onRTCError(const embree::RTCError, const char* str)
    {
        Engine::Log(LogType::EError, "CPURayRenderer", "Embree: " +  string(str));
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
        ray_o.geomID[i] = ray_i.geomID;
        ray_o.primID[i] = ray_i.primID;
        ray_o.instID[i] = ray_i.instID;
    }
}