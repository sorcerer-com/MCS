// CPURayRenderer.h
#pragma once

#include "Renderer.h"
#include "..\Utils\Header.h"
#include "..\Utils\RayUtils.h"
#include "..\Utils\Types\Vector3.h"

namespace embree {
    enum RTCError;
    struct RTCRay;
    struct __RTCScene;
}

namespace MyEngine {

    class SceneElement;
    using SceneElementPtr = shared_ptr < SceneElement >;

    class CPURayRenderer : public ProductionRenderer
    {
        // TODO: 
        // images(diffuse, specular, direct light, indirect light, total light, reflection, refraction, depth, final) - map may be?
        // buffer intersection infos
        // flags - max reflection/refraction level, GI, interactive(dynamic scene or not), animation - may be in ProductionRenderer
	public:
        uint RegionSize;
        vector<Region> Regions;

    private:
        Vector3 upLeft, dx, dy;
        Vector3 up, right, front;

        embree::__RTCScene* rtcScene;
        map<uint, embree::__RTCScene*> rtcGeometries; // mesh id / rtcScene(Geometry) id
        map<int, uint> rtcInstances; // rtcInstance id / scene element id

	public:
        CPURayRenderer(Engine* owner);
        ~CPURayRenderer();

        virtual vector<string> GetBufferNames() override;
        virtual bool Init(uint width, uint height) override;
        virtual void Start() override;
        virtual void Stop() override;


	private:
        void generateRegions();
        void beginFrame();
        embree::RTCRay getRTCScreenRay(float x, float y) const;

        void createRTCScene();
        embree::__RTCScene* createRTCGeometry(const SceneElementPtr sceneElement);

        static void onRTCError(const embree::RTCError code, const char* str);

	};

}