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

#include "webpwriter.h"

#include <cstdio>

#include <webp/encode.h>

#include <Libpfs/io/rgb8buffer.h>

namespace pfs {
namespace io {

WebpWriter::WebpWriter(const std::string &filename) : FrameWriter(filename) {}

WebpWriter::WebpWriter() : FrameWriter() {}

bool WebpWriter::write(const pfs::Frame &frame, const pfs::Params &params) {
    Rgb8WriterParams p;
    p.parse(params);
    std::vector<uint8_t> pixels = toRgb8(frame, p);

    const int width = int(frame.getWidth());
    const int height = int(frame.getHeight());
    // WebP is limited to 16383 x 16383 pixels
    if (width > WEBP_MAX_DIMENSION || height > WEBP_MAX_DIMENSION)
        throw WriteException("WebP: the image is larger than 16383 pixels");

    uint8_t *output = nullptr;
    const size_t size =
        p.quality_ >= 100
            ? WebPEncodeLosslessRGB(pixels.data(), width, height, width * 3,
                                    &output)
            : WebPEncodeRGB(pixels.data(), width, height, width * 3,
                            float(p.quality_), &output);
    if (size == 0 || output == nullptr) {
        WebPFree(output);
        throw WriteException("WebP: encoding failed");
    }
    m_fileSize = size;

    bool ok = true;
    if (!filename().empty()) {
        FILE *f = std::fopen(filename().c_str(), "wb");
        if (f == nullptr) {
            WebPFree(output);
            throw InvalidFile("Cannot open file " + filename());
        }
        ok = std::fwrite(output, 1, size, f) == size;
        ok = (std::fclose(f) == 0) && ok;
    }
    WebPFree(output);
    if (!ok) throw WriteException("WebP: cannot write " + filename());
    return true;
}

}  // io
}  // pfs
