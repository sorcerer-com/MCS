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

    protected:
        using ColorsMapType = map < string, Color4 >; // buffer name / color

        Vector3 upLeft, dx, dy;
        Vector3 up, right, front;
        Vector3 pos;
        float focalPlaneDist, fNumber;
        int nextRagion;

        embree::__RTCScene* rtcScene;
        map<uint, embree::__RTCScene*> rtcGeometries; // mesh id / rtcScene(Geometry)
        map<int, SceneElementPtr> rtcInstances; // rtcInstance id / scene element

        map<uint, ContentElementPtr> contentElementCache; // id / content element

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

        bool preview();
        bool render();
        uint adaptiveSampling(const function<Color4()>& func);
        Color4 renderPixel(int x, int y);
        ColorsMapType computeColor(const embree::RTCRay& rtcRay);
        bool postProcessing();

        static void onRTCError(const embree::RTCError code, const char* str);
        static void setRTCRay4(embree::RTCRay4& ray_o, int i, const embree::RTCRay& ray_i);
        static embree::RTCRay getRTCRay(const embree::RTCRay4& ray_i, int i);

	};

}