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

#include "lenscorrection.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

#include <lensfun.h>

#include <Libpfs/frame.h>

namespace pfs {
namespace io {

namespace {
// the database is read once and kept; Lensfun's lookups are read only
std::mutex g_dbMutex;
lfDatabase *g_db = nullptr;
std::string g_dbDir;

lfDatabase *database(const std::string &dir) {
    std::lock_guard<std::mutex> lock(g_dbMutex);
    if (g_db && g_dbDir == dir) return g_db;
    lfDatabase *db = new lfDatabase();
    bool loaded = false;
    if (!dir.empty()) loaded = db->LoadDirectory(dir.c_str());
    if (!loaded) loaded = db->Load() == LF_NO_ERROR;
    if (!loaded) {
        delete db;
        return nullptr;
    }
    g_db = db;  // kept for the life of the program
    g_dbDir = dir;
    return g_db;
}

//! bilinear sample of one channel at a position, edges clamped
float sample(const Channel &c, size_t w, size_t h, float x, float y) {
    x = std::min(std::max(x, 0.f), float(w - 1));
    y = std::min(std::max(y, 0.f), float(h - 1));
    const size_t x0 = size_t(x), y0 = size_t(y);
    const size_t x1 = std::min(x0 + 1, w - 1), y1 = std::min(y0 + 1, h - 1);
    const float ax = x - x0, ay = y - y0;
    const float top = c(x0, y0) * (1 - ax) + c(x1, y0) * ax;
    const float bottom = c(x0, y1) * (1 - ax) + c(x1, y1) * ax;
    return top * (1 - ay) + bottom * ay;
}
}  // namespace

bool correctLens(Frame &frame, const LensInfo &info,
                 const std::string &databaseDir, std::string *note) {
    lfDatabase *db = database(databaseDir);
    if (!db) {
        if (note) *note = "Lensfun database not found";
        return false;
    }
    if (info.lens.empty() || info.focal <= 0.f) {
        if (note) *note = "the RAW file names no lens";
        return false;
    }
    const lfCamera **cameras =
        db->FindCamerasExt(info.make.c_str(), info.model.c_str());
    const lfCamera *camera = cameras ? cameras[0] : nullptr;
    const lfLens **lenses = db->FindLenses(camera, nullptr, info.lens.c_str());
    const lfLens *lens = lenses ? lenses[0] : nullptr;
    if (!lens) {
        if (cameras) lf_free(cameras);
        if (lenses) lf_free(lenses);
        if (note) *note = "lens not in the Lensfun database: " + info.lens;
        return false;
    }
    const float crop = camera ? camera->CropFactor : lens->CropFactor;

    const size_t w = frame.getWidth(), h = frame.getHeight();
    Channel *r, *g, *b;
    frame.getXYZChannels(r, g, b);
    if (!r || !g || !b) {
        lf_free(cameras);
        lf_free(lenses);
        return false;
    }

#if defined(LF_VERSION) && LF_VERSION >= 0x00035f00
    lfModifier modifier(lens, info.focal, crop, int(w), int(h), LF_PF_F32);
    modifier.EnableDistortionCorrection();
    modifier.EnableTCACorrection();
    modifier.EnableVignettingCorrection(info.aperture, info.distance);
    lfModifier *mod = &modifier;
#else
    lfModifier *mod = lfModifier::Create(lens, crop, int(w), int(h));
    mod->Initialize(lens, LF_PF_F32, info.focal, info.aperture, info.distance,
                    1.0f, lens->Type,
                    LF_MODIFY_TCA | LF_MODIFY_VIGNETTING | LF_MODIFY_DISTORTION,
                    false);
#endif

    // vignetting, on interleaved linear RGB
    std::vector<float> rgb(w * h * 3);
    for (size_t i = 0; i < w * h; ++i) {
        rgb[i * 3] = (*r)(i);
        rgb[i * 3 + 1] = (*g)(i);
        rgb[i * 3 + 2] = (*b)(i);
    }
    for (size_t y = 0; y < h; ++y)
        mod->ApplyColorModification(&rgb[y * w * 3], 0.f, float(y), int(w), 1,
                                    LF_CR_3(RED, GREEN, BLUE),
                                    int(w * 3 * sizeof(float)));
    Channel src_r(w, h, "R"), src_g(w, h, "G"), src_b(w, h, "B");
    for (size_t i = 0; i < w * h; ++i) {
        src_r(i) = rgb[i * 3];
        src_g(i) = rgb[i * 3 + 1];
        src_b(i) = rgb[i * 3 + 2];
    }

    // distortion and colour fringes: where each colour of each pixel comes
    // from in the photo as taken
    std::vector<float> coords(w * 2 * 3);
    for (size_t y = 0; y < h; ++y) {
        if (!mod->ApplySubpixelGeometryDistortion(0.f, float(y), int(w), 1,
                                                  coords.data())) {
            for (size_t x = 0; x < w; ++x) {
                (*r)(x, y) = src_r(x, y);
                (*g)(x, y) = src_g(x, y);
                (*b)(x, y) = src_b(x, y);
            }
            continue;
        }
        for (size_t x = 0; x < w; ++x) {
            const float *c = &coords[x * 6];
            (*r)(x, y) = sample(src_r, w, h, c[0], c[1]);
            (*g)(x, y) = sample(src_g, w, h, c[2], c[3]);
            (*b)(x, y) = sample(src_b, w, h, c[4], c[5]);
        }
    }

#if !(defined(LF_VERSION) && LF_VERSION >= 0x00035f00)
    mod->Destroy();
#endif
    if (note) *note = std::string("corrected for ") + lens->Model;
    lf_free(cameras);
    lf_free(lenses);
    return true;
}

}  // io
}  // pfs
