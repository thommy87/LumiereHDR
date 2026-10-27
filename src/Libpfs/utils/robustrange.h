/*
 * This file is a part of LumiereHDR package.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#ifndef PFS_UTILS_ROBUSTRANGE_H
#define PFS_UTILS_ROBUSTRANGE_H

#include <algorithm>
#include <cmath>
#include <vector>

#include <Libpfs/array2d.h>

namespace pfs {
namespace utils {

//! the window (log10) an HDR picture is first shown with: the 0.1 and 99.5
//! percent points of \a image, at least half a decade wide. The very
//! brightest and darkest pixels (lamps, sensor noise) would leave the
//! picture far too dark.
inline void robustWindow(const Array2Df &image, float &logMin, float &logMax) {
    std::vector<float> values;
    const size_t size = image.getRows() * image.getCols();
    const size_t stride = std::max<size_t>(1, size / 200000);
    values.reserve(size / stride + 1);
    for (size_t i = 0; i < size; i += stride) {
        const float v = image(i);
        if (std::isfinite(v)) values.push_back(v);
    }
    float min = 0.000001f, max = 1.f;
    if (!values.empty()) {
        const size_t lo = values.size() / 1000;
        const size_t hi = values.size() - 1 - values.size() / 200;
        std::nth_element(values.begin(), values.begin() + lo, values.end());
        min = values[lo];
        std::nth_element(values.begin(), values.begin() + hi, values.end());
        max = values[hi];
    }
    if (max <= 0.000001f) max = 1.f;
    if (min <= 0.000001f) min = 0.000001f;  // negative values

    logMin = std::log10(min);
    logMax = std::log10(max);
    if (logMax - logMin < 0.5f) {  // window too small
        const float m = (logMin + logMax) / 2.f;
        logMax = m + 0.25f;
        logMin = m - 0.25f;
    }
}

}  // namespace utils
}  // namespace pfs

#endif  // PFS_UTILS_ROBUSTRANGE_H
