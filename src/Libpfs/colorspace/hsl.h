/*
 * This file is a part of LumiereHDR package.
 * ----------------------------------------------------------------------
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * ----------------------------------------------------------------------
 *
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

//! rief RGB <-> HSL, moved here from Common so the core needs no Qt

#ifndef PFS_COLORSPACE_HSL_H
#define PFS_COLORSPACE_HSL_H

#include <cmath>

#include <Libpfs/utils/minmax.h>

inline void rgb2hsl(float r, float g, float b, float &h, float &s, float &l) {
    float v, m, vm, r2, g2, b2;
    h = 0.0f;
    s = 0.0f;
    l = 0.0f;

    pfs::utils::minmax(r, g, b, m, v);

    l = (m + v) / 2.0f;
    if (l <= 0.0f) return;
    vm = v - m;
    s = vm;
    // if (s >= 0.0f)
    if (s > 0.0f) {
        float div = (l <= 0.5f) ? (v + m) : (2.0f - v - m);

        //assert(div != 0.f);

        if(std::fabs(div) < 1.e-15)
            div = 1.e-15;

        s /= div;
    } else {
        return;
    }
    r2 = (v - r) / vm;
    g2 = (v - g) / vm;
    b2 = (v - b) / vm;
    if (r == v)
        h = (g == m ? 5.0f + b2 : 1.0f - g2);
    else if (g == v)
        h = (b == m ? 1.0f + r2 : 3.0f - b2);
    else
        h = (r == m ? 3.0f + g2 : 5.0f - r2);
    h /= 6.0f;
}

inline void hsl2rgb(float h, float sl, float l, float &r, float &g, float &b) {
    float v;
    r = l;
    g = l;
    b = l;
    v = (l <= 0.5f) ? (l * (1.0f + sl)) : (l + sl - l * sl);
    if (v > 0.0f) {
        float m;
        float sv;
        int sextant;
        float fract, vsf, mid1, mid2;
        m = l + l - v;
        sv = (v - m) / v;
        h *= 6.0f;
        sextant = (int)h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;
        switch (sextant) {
            case 0:
                r = v;
                g = mid1;
                b = m;
                break;
            case 1:
                r = mid2;
                g = v;
                b = m;
                break;
            case 2:
                r = m;
                g = v;
                b = mid1;
                break;
            case 3:
                r = m;
                g = mid2;
                b = v;
                break;
            case 4:
                r = mid1;
                g = m;
                b = v;
                break;
            case 5:
                r = v;
                g = m;
                b = mid2;
                break;
        }
    }
}

#endif  // PFS_COLORSPACE_HSL_H
