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
    }

    CPURayRenderer::~CPURayRenderer()
    {
        this->thread->join();
        Engine::Log(LogType::ELog, "CPURayRenderer", "DeInit CPU Ray Renderer");
    }


    vector<string> CPURayRenderer::GetBufferNames()
    {
        return { "Diffuse", "Specular", "Reflection", "Refraction", "DirectLight", "IndirectLight", "TotalLight", "Depth", "Final" };
    }

    bool CPURayRenderer::Init(uint width, uint height)
    {
        ProfileLog;
        ProductionRenderer::Init(width, height);

        const auto& buffersNames = this->GetBufferNames();
        for (uint i = 0; i < buffersNames.size(); i++)
            this->Buffers[buffersNames[i]].init(width, height);

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
        this->thread->defThreadPool();

        // TODO: do preview and based on bucket render time sort them
        embree::rtcInit();
        this->beginFrame();
        this->createRTCScene();

        // TODO: remove
        embree::RTCRay ray = this->getRTCScreenRay(this->Width / 2, this->Height / 2);
        embree::rtcIntersect(this->rtcScene, ray);
        SceneElementPtr elem = this->Owner->SceneManager->GetElement(this->rtcInstances[ray.instID]);
        Engine::Log(LogType::ELog, "CPURayRenderer", elem->Name);

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

        this->thread->join();
        ProductionRenderer::Stop();

        Engine::Log(LogType::ELog, "CPURayRenderer", "Stop Rendering");
    }


    void CPURayRenderer::generateRegions()
    {
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
        embree::RTCSceneFlags sflags = embree::RTCSceneFlags::RTC_SCENE_STATIC | embree::RTCSceneFlags::RTC_SCENE_HIGH_QUALITY;
        embree::RTCAlgorithmFlags aflags = embree::RTCAlgorithmFlags::RTC_INTERSECT1 | embree::RTCAlgorithmFlags::RTC_INTERSECT4 | embree::RTCAlgorithmFlags::RTC_INTERSECT8 | embree::RTCAlgorithmFlags::RTC_INTERSECT16;
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

                this->rtcInstances[rtcInstance] = sceneElement->ID;
            }
        }
        embree::rtcCommit(this->rtcScene);
    }

    embree::RTCScene CPURayRenderer::createRTCGeometry(const SceneElementPtr sceneElement)
    {
        if (this->rtcGeometries.find(sceneElement->ContentID) != this->rtcGeometries.end())
            return this->rtcGeometries[sceneElement->ContentID];

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

        embree::RTCSceneFlags sflags = embree::RTCSceneFlags::RTC_SCENE_STATIC | embree::RTCSceneFlags::RTC_SCENE_HIGH_QUALITY;
        embree::RTCAlgorithmFlags aflags = embree::RTCAlgorithmFlags::RTC_INTERSECT1 | embree::RTCAlgorithmFlags::RTC_INTERSECT4 | embree::RTCAlgorithmFlags::RTC_INTERSECT8 | embree::RTCAlgorithmFlags::RTC_INTERSECT16;
        embree::RTCScene rtcGeometry = embree::rtcNewScene(sflags, aflags);

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


    void CPURayRenderer::onRTCError(const embree::RTCError, const char* str)
    {
        Engine::Log(LogType::EError, "CPURayRenderer", "Embree: " +  string(str));
    }

}