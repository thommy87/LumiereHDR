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

//! \brief Shared by the HDR AVIF and JPEG XL readers and writers: the PQ
//! and HLG curves, Rec. 709 <-> Rec. 2020, and how a merged HDR (relative
//! values) is placed on an absolute brightness scale.

#ifndef PFS_IO_HDRENCODING_H
#define PFS_IO_HDRENCODING_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <Libpfs/frame.h>

namespace pfs {
namespace io {
namespace hdr {

//! normal white, in nits (ITU-R BT.2408)
const float kReferenceWhite = 203.f;
//! the brightest highlights written, in nits
const float kPeak = 1000.f;

//! SMPTE ST 2084: nits (0..10000) -> signal 0..1
inline float pqEncode(float nits) {
    const float m1 = 2610.f / 16384.f, m2 = 2523.f / 4096.f * 128.f;
    const float c1 = 3424.f / 4096.f, c2 = 2413.f / 4096.f * 32.f,
                c3 = 2392.f / 4096.f * 32.f;
    const float y = std::pow(std::min(std::max(nits, 0.f), 10000.f) / 10000.f, m1);
    return std::pow((c1 + c2 * y) / (1.f + c3 * y), m2);
}

//! signal 0..1 -> nits
inline float pqDecode(float signal) {
    const float m1 = 2610.f / 16384.f, m2 = 2523.f / 4096.f * 128.f;
    const float c1 = 3424.f / 4096.f, c2 = 2413.f / 4096.f * 32.f,
                c3 = 2392.f / 4096.f * 32.f;
    const float e = std::pow(std::min(std::max(signal, 0.f), 1.f), 1.f / m2);
    return 10000.f *
           std::pow(std::max(e - c1, 0.f) / (c2 - c3 * e), 1.f / m1);
}

//! ITU-R BT.2100 HLG signal 0..1 -> scene light 0..1
inline float hlgDecode(float signal) {
    const float a = 0.17883277f, b = 0.28466892f, c = 0.55991073f;
    signal = std::min(std::max(signal, 0.f), 1.f);
    return signal <= 0.5f ? signal * signal / 3.f
                          : (std::exp((signal - c) / a) + b) / 12.f;
}

//! linear Rec. 709 -> linear Rec. 2020 (same white)
inline void rec709To2020(float &r, float &g, float &b) {
    const float r2 = 0.6274f * r + 0.3293f * g + 0.0433f * b;
    const float g2 = 0.0691f * r + 0.9195f * g + 0.0114f * b;
    const float b2 = 0.0164f * r + 0.0880f * g + 0.8956f * b;
    r = r2;
    g = g2;
    b = b2;
}

//! linear Rec. 2020 -> linear Rec. 709
inline void rec2020To709(float &r, float &g, float &b) {
    const float r7 = 1.6605f * r - 0.5876f * g - 0.0728f * b;
    const float g7 = -0.1246f * r + 1.1329f * g - 0.0083f * b;
    const float b7 = -0.0182f * r - 0.1006f * g + 1.1187f * b;
    r = r7;
    g = g7;
    b = b7;
}

//! The merged HDR in nits, Rec. 2020, interleaved RGB: its log-average
//! brightness lands on mid grey (18 % of normal white), highlights go
//! above and roll off softly towards kPeak.
inline std::vector<float> toNits2020(const Frame &frame) {
    const Channel *r, *g, *b;
    frame.getXYZChannels(r, g, b);
    const size_t size = frame.getWidth() * frame.getHeight();
    std::vector<float> out(size * 3, 0.f);
    if (!r || !g || !b) return out;

    double sum = 0;
    size_t n = 0;
    const size_t stride = std::max<size_t>(1, size / 200000);
    for (size_t i = 0; i < size; i += stride) {
        const float y = 0.2126f * (*r)(i) + 0.7152f * (*g)(i) + 0.0722f * (*b)(i);
        if (std::isfinite(y) && y > 1e-6f) {
            sum += std::log(double(y));
            ++n;
        }
    }
    const float average = n ? float(std::exp(sum / double(n))) : 0.18f;
    const float scale = 0.18f * kReferenceWhite / average;
    const float knee = 0.75f * kPeak;

    for (size_t i = 0; i < size; ++i) {
        float c[3] = {std::max((*r)(i), 0.f) * scale,
                      std::max((*g)(i), 0.f) * scale,
                      std::max((*b)(i), 0.f) * scale};
        const float y = 0.2126f * c[0] + 0.7152f * c[1] + 0.0722f * c[2];
        if (y > knee) {
            const float rolled =
                kPeak - (kPeak - knee) * std::exp(-(y - knee) / (kPeak - knee));
            for (float &v : c) v *= rolled / y;
        }
        rec709To2020(c[0], c[1], c[2]);
        for (int k = 0; k < 3; ++k)
            out[i * 3 + k] = std::min(std::max(c[k], 0.f), kPeak);
    }
    return out;
}

}  // namespace hdr
}  // namespace io
}  // namespace pfs

#endif  // PFS_IO_HDRENCODING_H
