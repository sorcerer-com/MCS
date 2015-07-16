// RayUtils.h
#pragma once

#include "Types\Vector3.h"
#include "Types\Quaternion.h"

namespace MyEngine {

    class SceneElement;
    using SceneElementPtr = shared_ptr < SceneElement >;

    struct Region
    {
        int x, y, w, h;
        bool active;
        float time;

        Region(int _x, int _y, int _w, int _h)
        {
            x = _x;
            y = _y;
            w = _w;
            h = _h;
            active = false;
            time = 0;
        }

        Region(const Region& reg)
        {
            x = reg.x;
            y = reg.y;
            w = reg.w;
            h = reg.h;
            active = reg.active;
            time = reg.time;
        }
    };

    struct InterInfo
    {
        SceneElementPtr sceneElement;
        Vector3 UV;
        Vector3 interPos;

        Color4 color;
        Vector3 normal;
        // TODO: reflection / refraction pdf
    };


    inline vector<float> getMatrix(const Vector3& pos, Quaternion rot, const Vector3& scl)
    {
        vector<float> result(16);
        result[0] = (1.0f - 2.0f * rot.y * rot.y - 2.0f * rot.z * rot.z) * scl.x;
        result[1] = (2.0f * rot.x * rot.y + 2.0f * rot.z * rot.w) * scl.x;
        result[2] = (2.0f * rot.x * rot.z - 2.0f * rot.y * rot.w) * scl.x;
        result[3] = 0.0f;

        result[4] = (2.0f * rot.x * rot.y - 2.0f * rot.z * rot.w) * scl.y;
        result[5] = (1.0f - 2.0f * rot.x * rot.x - 2.0f * rot.z * rot.z) * scl.y;
        result[6] = (2.0f * rot.z * rot.y + 2.0f * rot.x * rot.w) * scl.y;
        result[7] = 0.0f;

        result[8] = (2.0f * rot.x * rot.z + 2.0f * rot.y * rot.w) * scl.z;
        result[9] = (2.0f * rot.z * rot.y - 2.0f * rot.x * rot.w) * scl.z;
        result[10] = (1.0f - 2.0f * rot.x * rot.x - 2.0f * rot.y * rot.y) * scl.z;
        result[11] = 0.0f;

        result[12] = pos.x;
        result[13] = pos.y;
        result[14] = pos.z;
        result[15] = 1.0f;
        return result;
    }

    inline Vector3 perpendicularVector(const Vector3& vec)
    {
        int max_compo = vec.maxDim();

        switch (max_compo) {
        case  0: return Vector3(-vec.y / vec.x, 1.0f, 0.0f);
        case  1: return Vector3(0.0f, -vec.z / vec.y, 1.0f);
        case  2: return Vector3(0.0f, 1.0f, -vec.y / vec.z);
        case -1:
        default:
            return Vector3(0.0f, 0.0f, 0.0f);
        }
    }

}