// CPURayRenderer.h
#pragma once

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
        int nextRagion;

        embree::__RTCScene* rtcScene;
        map<uint, embree::__RTCScene*> rtcGeometries; // mesh id / rtcScene(Geometry) id
        map<int, SceneElementPtr> rtcInstances; // rtcInstance id / scene element

	public:
        CPURayRenderer(Engine* owner);
        ~CPURayRenderer();

        virtual vector<string> GetBufferNames() override;
        virtual vector<Region> GetActiveRegions() override;
        virtual bool Init(uint width, uint height) override;
        virtual void Start() override;
        virtual void Stop() override;


	private:
        void generateRegions();
        void beginFrame();
        embree::RTCRay getRTCScreenRay(float x, float y) const;

        void createRTCScene();
        embree::__RTCScene* createRTCGeometry(const SceneElementPtr sceneElement);

        bool render(bool preview);
        bool sortRegions();

        static void onRTCError(const embree::RTCError code, const char* str);
        static void setRTCRay4(embree::RTCRay4& ray_o, int i, const embree::RTCRay& ray_i);

	};

}