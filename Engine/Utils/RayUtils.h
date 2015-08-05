// RayUtils.h
#pragma once

#include "Types\Random.h"
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
        Vector3 interPos;
        Vector3 UV;

        Color4 color;
        Vector3 normal;

        float diffuse;
        float refraction;
        float reflection;
    };

    
    enum RayFlags
    {
        RAY_INDIRECT = (1 << 0),
        RAY_INSIDE = (1 << 1),
    };

    inline bool getFlag(uint flags, uint flag)
    {
        return (flags & flag) != 0;
    }

    inline bool getFlag(float flags, uint flag)
    {
        return getFlag((uint)flags, flag);
    }

    inline void setFlag(uint& flags, uint flag, bool value)
    {
        if (value)
            flags |= flag;
        else
            flags &= ~flag;
    }

    inline void setFlag(float& flags, uint flag, bool value)
    {
        uint _flags = (uint)flags;
        setFlag(_flags, flag, value);
        flags = (float)_flags;
    }


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


    inline Vector3 faceforward(const Vector3& ray, const Vector3& norm)
    {
        if (dot(ray, norm) < 0.0f) return norm;
        else return -norm;
    }

    inline Vector3 reflect(const Vector3& ray, const Vector3& norm)
    {
        Vector3 result = ray - dot(ray, norm) * norm * 2.0f;
        result.normalize();
        return result;
    }

    inline Vector3 refract(const Vector3& i, const Vector3& n, float ior)
    {
        float NdotI = dot(i, n);
        float k = 1.0f - (ior * ior) * (1.0f - NdotI * NdotI);
        if (k < 0.0f) // Check for total inner reflection
            return Vector3(0.0f, 0.0f, 0.0f);
        Vector3 result = i * ior - n * (ior * NdotI + sqrt(k));
        result.normalize();
        return result;
    }

    void orthonormedSystem(const Vector3& a, Vector3& b, Vector3& c);
    inline Vector3 glossy(const Vector3& n, float glossiness)
    {
        Random& rand = Random::getRandomGen();
        Vector3 pn1, pn2;
        orthonormedSystem(n, pn1, pn2);
        float x, y;
        rand.unitDiscSample(x, y);
        Vector3 newN = n + (pn1 * x + pn2 * y) * tan((1.0f -  glossiness) * PI / 2.0f);
        newN.normalize();
        return newN;
    }

    // Use the Schlick's approximation to evaluate the fresnel coefficient
    // for an incident ray `i', normal `n' and the given ior.
    // The coefficient represents Reflection / (Reflection + Refraction)
    inline float fresnel(const Vector3& i, const Vector3& n, float ior)
    {
        float f = (1.0f - ior) / (1.0f + ior);
        f = f * f;
        float NdotI = -dot(n, i);
        return f + (1.0f - f) * pow(1.0f - NdotI, 5.0f);
    }

    inline void orthonormedSystem(const Vector3& a, Vector3& b, Vector3& c)
    {
        Vector3 temp = Vector3(1, 0, 0);
        if (abs(dot(a, temp)) > 0.99f) {
            temp = Vector3(0, 1, 0);
            if (abs(dot(a, temp)) > 0.99f)
                temp = Vector3(0, 0, 1);
        }
        b = cross(a, temp);
        b.normalize();
        c = cross(a, b);
        c.normalize();
    }

    inline Vector3 hemisphereSample(const Vector3& normal)
    {
        Random& rand = Random::getRandomGen();
        float z = rand.randSample(20) * 2.0f - 1.0f;
        float t = rand.randSample(36) * 2.0f * PI;
        float r = sqrt(1.0f - z * z);
        
        Vector3 res;
        res.x = r * cos(t);
        res.y = r * sin(t);
        res.z = z;
        res.normalize();

        if (dot(res, normal) < 0)
            res = -res;
        return res;
    }

}