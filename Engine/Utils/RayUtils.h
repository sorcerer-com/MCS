// RayUtils.h
#pragma once

namespace MyEngine {

    struct Region
    {
        int x, y, w, h;

        Region(const Region& reg)
        {
            x = reg.x;
            y = reg.y;
            w = reg.w;
            h = reg.h;
        }

        Region(int _x, int _y, int _w, int _h)
        {
            x = _x;
            y = _y;
            w = _w;
            h = _h;
        }
    };

}