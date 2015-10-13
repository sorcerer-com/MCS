// CPURayRenderer.h
#pragma once

#include <functional>

#include "Renderer.h"
#include "..\Utils\Header.h"
#include "..\Utils\RayUtils.h"
#include "..\Utils\Types\Vector3.h"
#include "..\Utils\Types\KdTree.h"

namespace embree {
    enum RTCError;
    struct RTCRay;
    struct RTCRay4;
    struct __RTCScene;
}

namespace MyEngine {

    struct Color4;
    class SceneElement;
    using SceneElementPtr = shared_ptr < SceneElement >;
    class Light;
    class ContentElement;
    using ContentElementPtr = shared_ptr < ContentElement >;

    class CPURayRenderer : public ProductionRenderer
    {
	public:
        uint RegionSize;
        vector<Region> Regions;
        bool VolumetricFog;
        uint MinSamples, MaxSamples;
        float SampleThreshold;
        uint MaxLights;
        uint MaxDepth;
        bool GI;
        uint GISamples;
        bool IrradianceMap;
        uint IrradianceMapSamples;
        float IrradianceMapDistanceThreshold;
        float IrradianceMapNormalThreshold;
        float IrradianceMapColorThreshold;
        bool LightCache;
        float LightCacheSampleSize;

    protected:
        using ColorsMapType = map < string, Color4 >; // buffer name / color
        static const uint RAYS = 4;
        static const int VALID[RAYS];

        Vector3 upLeft, dx, dy;
        Vector3 up, right, front;
        Vector3 pos;
        float focalPlaneDist, fNumber;
        int nextRagion;

        embree::__RTCScene* rtcScene;
        embree::__RTCScene* rtcSystemScene;
        map<uint, embree::__RTCScene*> rtcGeometries; // mesh id / rtcScene(Geometry)
        map<int, SceneElementPtr> rtcInstances; // rtcInstance id / scene element

        map<uint, ContentElementPtr> contentElements; // id / content element
        vector<SceneElementPtr> lights;

        vector<IrradianceMapSample> irrMapSamples;
        vector<int> irrMapTriangles;
        embree::__RTCScene* rtcIrrMapScene;
        vector<LightCacheSample> lightCacheSamples;
        KdTree<Vector3> lightCacheKdTree;

        shared_ptr<Profiler> phasePofiler;

	public:
        CPURayRenderer(Engine* owner);
        CPURayRenderer& operator=(const CPURayRenderer&) { return *this; }
        ~CPURayRenderer();

        virtual vector<string> GetBufferNames() override;
        virtual vector<Region> GetActiveRegions() override;
        virtual double GetProgress() override;
        virtual bool Init(uint width, uint height) override;
        virtual void Start() override;
        virtual void Stop() override;


	protected:
        void generateRegions();
        bool sortRegions();
        void beginFrame();
        embree::RTCRay getRTCScreenRay(float x, float y) const;

        void createRTCScene();
        embree::__RTCScene* createRTCGeometry(const SceneElementPtr sceneElement);
        void cacheContentElements(const SceneElementPtr sceneElement);
        InterInfo getInterInfo(const embree::RTCRay& rtcRay, bool onlyColor = false, bool noNormalMap = false);
        void processRenderElements(embree::RTCRay& rtcRay, InterInfo& interInfo);

        bool generateIrradianceMap();
        bool addIrradianceMapSample(float x, float y, KdTree<Vector3>& irrKdTree, float minDist);
        void addIrradianceMapTriangle(int v1, int v2, int v3);
        bool computeIrradianceMap();

        bool render(bool preview);
        Color4 renderPixel(int x, int y);
        ColorsMapType computeColor(const embree::RTCRay& rtcRay, const InterInfo& interInfo, float contribution);
        ColorsMapType getLighting(const embree::RTCRay& rtcRay, const InterInfo& interInfo); // diffuse light / sepcular light / samples
        ColorsMapType getLighting(const embree::RTCRay& rtcRay, const Light* light, const InterInfo& interInfo); // diffuse light / sepcular light
        Vector3 getLightSample(const Light* light, int numSamples, int sample);
        Color4 getFogLighting(const embree::RTCRay& rtcRay);
        Color4 getGILighting(const embree::RTCRay& rtcRay, const InterInfo& interInfo, const Color4& pathMultiplier);
        bool postProcessing();

        static void onRTCError(const embree::RTCError code, const char* str);
        static embree::RTCRay RTCRay(const Vector3& start, const Vector3& dir, uint depth, float near = 0.01f, float far = 10000.0f);
        static void setRTCRay4(embree::RTCRay4& ray_o, int i, const embree::RTCRay& ray_i);
        static embree::RTCRay getRTCRay(const embree::RTCRay4& ray_i, int i);
        template <typename Func>
        static uint adaptiveSampling(uint minSamples, uint maxSamples, float sampleThreshold, const Func& func);

	};

}