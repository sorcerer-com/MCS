// CPURayRenderer.h
#pragma once

#include <functional>

#include "Renderer.h"
#include "..\Utils\Header.h"
#include "..\Utils\RayUtils.h"
#include "..\Utils\Types\Vector3.h"

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
        // TODO: 
        // images(diffuse, specular, direct light, indirect light, total light, reflection, refraction, depth, final) - map may be?
        // buffer intersection infos
        // flags - max reflection/refraction level, GI, interactive(dynamic scene or not), animation - may be in ProductionRenderer
	public:
        uint RegionSize;
        vector<Region> Regions;
        uint MinSamples, MaxSamples;
        float SamplesThreshold;
        uint MaxLights;
        uint MaxDepth;

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
        map<uint, embree::__RTCScene*> rtcGeometries; // mesh id / rtcScene(Geometry)
        map<int, SceneElementPtr> rtcInstances; // rtcInstance id / scene element

        map<uint, ContentElementPtr> contentElementCache; // id / content element
        vector<SceneElementPtr> lightsCache;

        shared_ptr<Profiler> phasePofiler;

	public:
        CPURayRenderer(Engine* owner);
        ~CPURayRenderer();

        virtual vector<string> GetBufferNames() override;
        virtual vector<Region> GetActiveRegions() override;
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
        InterInfo getInterInfo(const embree::RTCRay& rtcRay, bool onlyColor = false);

        bool render(bool preview);
        Color4 renderPixel(int x, int y);
        ColorsMapType computeColor(const embree::RTCRay& rtcRay, const InterInfo& interInfo);
        ColorsMapType getLighting(const embree::RTCRay& rtcRay, const InterInfo& interInfo); // diffuse light / sepcular light / samples
        ColorsMapType getLighting(const embree::RTCRay& rtcRay, const Light* light, const InterInfo& interInfo); // diffuse light / sepcular light
        Vector3 getLightSample(const Light* light, int numSamples, int sample);
        Color4 getFogLighting(const embree::RTCRay& rtcRay, float fogFactor);
        bool postProcessing();

        static void onRTCError(const embree::RTCError code, const char* str);
        static embree::RTCRay RTCRay(const Vector3& start, const Vector3& dir, uint depth, float near = 0.1f, float far = 10000.0f);
        static void setRTCRay4(embree::RTCRay4& ray_o, int i, const embree::RTCRay& ray_i);
        static embree::RTCRay getRTCRay(const embree::RTCRay4& ray_i, int i);
        static uint adaptiveSampling(uint minSamples, uint maxSamples, float samplesThreshold, const function<Color4(int)>& func);

	};

}