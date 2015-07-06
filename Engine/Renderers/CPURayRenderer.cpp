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
#include "..\Content Elements\Mesh.h"
#include "..\Content Elements\Texture.h"


namespace MyEngine {

    pair<unsigned, Random> Random::rg_table[Random::RGENS];
    
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
        this->thread->addNTasks([&](int) { return this->preview(); }, (int)this->Regions.size());
        this->thread->addWaitTask();
        this->thread->addTask([&](int) { return this->sortRegions(); });
        this->thread->addWaitTask();
        this->thread->addTask([&](int) { Engine::Log(LogType::ELog, "CPURayRenderer", duration_to_string(this->phasePofiler->stop()) + " Preview phase time"); return true; });
        // render phase
        this->thread->addNTasks([&](int) { return this->render(); }, (int)(this->Regions.size() + this->thread->workersCount() * 3 * 2));
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

        sort(this->Regions.begin(), this->Regions.end(), [](const Region& a, const Region& b) -> bool
        {
            const float delta = 0.001f;
            float aComplexity = (a.time * a.time) / 1000;
            float bComplexity = (b.time * b.time) / 1000;

            if (abs(aComplexity - bComplexity) >= delta)
                return aComplexity < bComplexity;
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

            dx *= 10.0f / this->fNumber; // TODO: get from fmiray is it ok?
            dy *= 10.0f / this->fNumber;

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
            if (sceneElement->ContentID == INVALID_ID || !sceneElement->Visible)
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


    bool CPURayRenderer::preview()
    {
        const int delta = 4;
        if (this->nextRagion >= this->Regions.size())
            return false;

        // get region
        this->thread->mutex("regions").lock();
        Region& region = this->Regions[this->nextRagion];
        this->nextRagion++;
        this->thread->mutex("regions").unlock();

        Profiler prof;
        prof.start();

        // render
        for (int j = 0; j < region.h; j += delta)
        {
            for (int i = 0; i < region.w; i += delta)
            {
                if (!this->IsStarted)
                    return false;

                int x = region.x + i;
                int y = region.y + j;

                embree::RTCRay rtcRay = this->getRTCScreenRay((float)x, (float)y);
                embree::rtcIntersect(this->rtcScene, rtcRay);

                const ColorsMapType& colors = this->computeColor(rtcRay);

                // do preview
                for (int p = 0; p < delta * delta; p++)
                {
                    uint xx = region.x + i + p % delta;
                    uint yy = region.y + j + p / delta;
                    for (auto& color : colors)
                        this->Buffers[color.first].setElement(xx, yy, color.second);
                }
            }
        }

        region.time = (float)chrono::duration_cast<chrono::milliseconds>(prof.stop()).count();

        return true;
    }

    bool CPURayRenderer::render()
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
        region.active = true;

        // render
        for (int j = 0; j < region.h; j++)
        {
            for (int i = 0; i < region.w; i++)
            {
                if (!this->IsStarted)
                    return false;

                int x = region.x + i;
                int y = region.y + j;

                for (auto& buffer : this->Buffers)
                    buffer.second.setElement(x, y, Color4());

                uint samples = this->adaptiveSampling([&] {return this->renderPixel(x, y); });

                float div = 1.0f / samples;
                for (auto& buffer : this->Buffers)
                    buffer.second.setElement(x, y, this->Buffers[buffer.first].getElement(x, y) * div);
            }
        }

        region.active = false;
        region.time = (float)chrono::duration_cast<chrono::milliseconds>(prof.stop()).count();

        return true;
    }

    uint CPURayRenderer::adaptiveSampling(const function<Color4()>& func)
    {
        Color4 color;
        uint samples = 0;
        for (samples = 1; samples <= this->MaxSamples; samples++)
        {
            Color4 c = func();

            if (samples > this->MinSamples)
            {
                Color4 tempColor = color + c;
                Color4 c1 = color * (1.0f / (samples - 1));
                Color4 c2 = tempColor * (1.0f / samples);
                if (absolute(c1 - c2) < this->SamplesThreshold)
                    break;
            }
            color += c;
        }
        return std::min(samples, this->MaxSamples);
    }

    Color4 CPURayRenderer::renderPixel(int x, int y)
    {
        const uint RAYS = 4;
        Random& rand = Random::getRandomGen();

        if (!this->IsStarted)
            return Color4();

        // intersect
        const int valid4[4] = { -1, -1, -1, -1 };
        embree::RTCRay4 rtcRay4;
        for (int k = 0; k < RAYS; k++)
            setRTCRay4(rtcRay4, k, this->getRTCScreenRay(x + rand.randSample(10), y + rand.randSample(10)));
        embree::rtcIntersect4(valid4, this->rtcScene, rtcRay4);

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
        
        for (auto& color : colors)
            this->Buffers[color.first].setElement(x, y, this->Buffers[color.first].getElement(x, y) + color.second * (1.0f / RAYS));
        
        return colors["Final"] * (1.0f / RAYS);
    }

    using ColorsMapType = map < string, Color4 >; // buffer name / color
    ColorsMapType CPURayRenderer::computeColor(const embree::RTCRay& rtcRay)
    {
        ColorsMapType result;

        if (!this->IsStarted)
            return result;

        const SceneElementPtr& sceneElement = this->rtcInstances[rtcRay.instID];
        if (!sceneElement)
            return result;

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

        // "Diffuse", "Specular", "Reflection", "Refraction", "DirectLight", "IndirectLight", "TotalLight", "Depth", "Final"
        result["TotalLight"] = result["DirectLight"] + result["IndirectLight"];
        result["TotalLight"] = Color4(1.0f, 1.0f, 1.0f, 1.0f);

        float depth = rtcRay.tfar;
        result["Depth"] = Color4(depth, depth, depth);

        float refract = 0.0f;
        float reflect = 0.0f;
        result["Final"] = (result["Diffuse"] * result["TotalLight"] /*TODO: + specular*/) * 1.0f;
        result["Final"] = result["Final"] * (1.0f - refract) + result["Refraction"] * refract;
        result["Final"] = result["Final"] * (1.0f - reflect) + result["Reflection"] * reflect;
        // TODO: may be add fog buffer?

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

}