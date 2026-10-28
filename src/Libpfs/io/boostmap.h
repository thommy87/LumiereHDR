/*
 * This file is a part of LumiereHDR package.
 * ----------------------------------------------------------------------
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 * ----------------------------------------------------------------------
 */

//! \brief How much brighter than the edited result each part of a picture
//! may glow on an HDR screen; shared by Ultra HDR and the HDR AVIF and
//! JPEG XL made from a result.

#ifndef PFS_IO_BOOSTMAP_H
#define PFS_IO_BOOSTMAP_H

#include <algorithm>
#include <cmath>
#include <vector>

#include <Libpfs/frame.h>
#include <Libpfs/io/ioexception.h>

namespace pfs {
namespace io {

const float kMaxBoost = 8.f;  // three stops above the result's white

//! Boost per pixel of a coarse grid over the image, from the scene's
//! luminance in the HDR: 1 up to one stop above the scene's median, then
//! rising (in stops) to the full boost at the brightest 1 % of the scene.
//! Each picture's brightest parts therefore glow fully, an overcast sky as
//! much as a sunny one, while shadows never do. Smoothed so that no edge,
//! texture or noise shows in it.
class BoostMap {
   public:
    BoostMap(const pfs::Frame &hdr, size_t width, size_t height)
        : m_width(width), m_height(height) {
        const pfs::Channel *r, *g, *b;
        hdr.getXYZChannels(r, g, b);
        if (!r || !g || !b)
            throw WriteException("Ultra HDR: the HDR image has no colour data");
        m_cell = std::max<size_t>(1, std::max(width, height) / 256);
        m_gw = (width + m_cell - 1) / m_cell;
        m_gh = (height + m_cell - 1) / m_cell;
        m_grid.assign(m_gw * m_gh, 0.f);
        std::vector<int> count(m_grid.size(), 0);
        for (size_t y = 0; y < height; ++y)
            for (size_t x = 0; x < width; ++x) {
                const size_t i = y * width + x;
                const float lum =
                    0.2126f * (*r)(i) + 0.7152f * (*g)(i) + 0.0722f * (*b)(i);
                const size_t c = (y / m_cell) * m_gw + x / m_cell;
                m_grid[c] += std::max(lum, 0.f);
                ++count[c];
            }
        for (size_t c = 0; c < m_grid.size(); ++c)
            m_grid[c] /= std::max(count[c], 1);

        std::vector<float> sorted(m_grid);
        std::sort(sorted.begin(), sorted.end());
        const float median = sorted[sorted.size() / 2];
        const float top = sorted[std::min(sorted.size() - 1,
                                          sorted.size() * 99 / 100)];
        const float start = 2.f * median;
        // never stretch a small range into the full boost
        const float span = std::max(std::log2(std::max(top, 1e-12f) /
                                              std::max(start, 1e-12f)),
                                    1.f);
        for (float &v : m_grid) {
            float t = 0.f;
            if (start > 0.f && v > start) t = std::log2(v / start) / span;
            t = std::min(std::max(t, 0.f), 1.f);
            v = std::pow(kMaxBoost, t);
        }
        blur();
    }

    //! boost at an image pixel, bilinear between cell centres
    float at(float x, float y) const {
        const float fx = std::min(std::max((x + 0.5f) / m_cell - 0.5f, 0.f),
                                  float(m_gw - 1));
        const float fy = std::min(std::max((y + 0.5f) / m_cell - 0.5f, 0.f),
                                  float(m_gh - 1));
        const size_t x0 = size_t(fx), y0 = size_t(fy);
        const size_t x1 = std::min(x0 + 1, m_gw - 1);
        const size_t y1 = std::min(y0 + 1, m_gh - 1);
        const float ax = fx - x0, ay = fy - y0;
        const float top =
            m_grid[y0 * m_gw + x0] * (1 - ax) + m_grid[y0 * m_gw + x1] * ax;
        const float bottom =
            m_grid[y1 * m_gw + x0] * (1 - ax) + m_grid[y1 * m_gw + x1] * ax;
        return top * (1 - ay) + bottom * ay;
    }

   private:
    // three box blurs in each direction, close to a gaussian
    void blur() {
        std::vector<float> tmp(m_grid.size());
        const int radius = 2;
        for (int pass = 0; pass < 3; ++pass) {
            for (size_t y = 0; y < m_gh; ++y)
                for (size_t x = 0; x < m_gw; ++x) {
                    float sum = 0.f;
                    int k = 0;
                    for (int d = -radius; d <= radius; ++d) {
                        const long xx = long(x) + d;
                        if (xx < 0 || xx >= long(m_gw)) continue;
                        sum += m_grid[y * m_gw + size_t(xx)];
                        ++k;
                    }
                    tmp[y * m_gw + x] = sum / k;
                }
            for (size_t y = 0; y < m_gh; ++y)
                for (size_t x = 0; x < m_gw; ++x) {
                    float sum = 0.f;
                    int k = 0;
                    for (int d = -radius; d <= radius; ++d) {
                        const long yy = long(y) + d;
                        if (yy < 0 || yy >= long(m_gh)) continue;
                        sum += tmp[size_t(yy) * m_gw + x];
                        ++k;
                    }
                    m_grid[y * m_gw + x] = sum / k;
                }
        }
    }

    size_t m_width, m_height, m_cell = 1, m_gw = 1, m_gh = 1;
    std::vector<float> m_grid;
};

}  // io
}  // pfs

#endif  // PFS_IO_BOOSTMAP_H
