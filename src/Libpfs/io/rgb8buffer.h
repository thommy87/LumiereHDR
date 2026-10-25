/*
 * This file is a part of LumiereHDR package.
 * ----------------------------------------------------------------------
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * ----------------------------------------------------------------------
 */

//! \brief Shared by the AVIF and JPEG XL writers: settings common to all
//! LDR writers, and the frame as interleaved 8-bit RGB.

#ifndef PFS_IO_RGB8BUFFER_H
#define PFS_IO_RGB8BUFFER_H

#include <cstdint>
#include <vector>

#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/colorspace/rgbremapper.h>
#include <Libpfs/fixedstrideiterator.h>
#include <Libpfs/frame.h>
#include <Libpfs/params.h>
#include <Libpfs/utils/chain.h>
#include <Libpfs/utils/clamp.h>
#include <Libpfs/utils/transform.h>

namespace pfs {
namespace io {

struct Rgb8WriterParams {
    size_t quality_ = 90;  // 1..100, 100 = lossless
    float minLuminance_ = 0.f;
    float maxLuminance_ = 1.f;
    RGBMappingType luminanceMapping_ = MAP_LINEAR;

    void parse(const Params &params) {
        for (Params::const_iterator it = params.begin(), itEnd = params.end();
             it != itEnd; ++it) {
            if (it->first == "quality")
                quality_ = it->second.as<size_t>(quality_);
            else if (it->first == "min_luminance")
                minLuminance_ = it->second.as<float>(minLuminance_);
            else if (it->first == "max_luminance")
                maxLuminance_ = it->second.as<float>(maxLuminance_);
            else if (it->first == "mapping_method")
                luminanceMapping_ =
                    it->second.as<RGBMappingType>(luminanceMapping_);
        }
        if (quality_ < 1) quality_ = 1;
        if (quality_ > 100) quality_ = 100;
    }
};

//! the frame's RGB channels as R,G,B bytes, row after row
inline std::vector<uint8_t> toRgb8(const Frame &frame,
                                   const Rgb8WriterParams &params) {
    const size_t width = frame.getWidth();
    const size_t height = frame.getHeight();
    std::vector<uint8_t> out(width * height * 3);

    const Channel *r;
    const Channel *g;
    const Channel *b;
    frame.getXYZChannels(r, g, b);
    for (size_t row = 0; row < height; ++row) {
        uint8_t *line = out.data() + row * width * 3;
        utils::transform(
            r->row_begin(row), r->row_end(row), g->row_begin(row),
            b->row_begin(row), FixedStrideIterator<uint8_t *, 3>(line),
            FixedStrideIterator<uint8_t *, 3>(line + 1),
            FixedStrideIterator<uint8_t *, 3>(line + 2),
            utils::chain(colorspace::Normalizer(params.minLuminance_,
                                                params.maxLuminance_),
                         utils::CLAMP_F32,
                         Remapper<uint8_t>(params.luminanceMapping_)));
    }
    return out;
}

}  // io
}  // pfs

#endif
