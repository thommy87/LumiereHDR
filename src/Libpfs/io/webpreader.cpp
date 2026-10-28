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

#include "webpreader.h"

#include <cstdio>

#include <webp/decode.h>

#include <Libpfs/frame.h>

namespace pfs {
namespace io {

namespace {
//! the whole file, or nothing if it cannot be read
std::vector<uint8_t> readFile(const std::string &name) {
    std::vector<uint8_t> data;
    FILE *f = std::fopen(name.c_str(), "rb");
    if (!f) return data;
    uint8_t chunk[65536];
    size_t n;
    while ((n = std::fread(chunk, 1, sizeof(chunk), f)) > 0)
        data.insert(data.end(), chunk, chunk + n);
    std::fclose(f);
    return data;
}
}  // namespace

WebpReader::WebpReader(const std::string &filename) : FrameReader(filename) {
    open();
}

void WebpReader::open() {
    m_file = readFile(filename());
    int width = 0, height = 0;
    if (m_file.empty() ||
        !WebPGetInfo(m_file.data(), m_file.size(), &width, &height))
        throw InvalidFile("Cannot read the WebP file " + filename());
    setWidth(size_t(width));
    setHeight(size_t(height));
}

bool WebpReader::isOpen() const { return !m_file.empty(); }

void WebpReader::close() { m_file.clear(); }

void WebpReader::read(Frame &frame, const Params &params) {
    if (!isOpen()) open();
    int width = 0, height = 0;
    uint8_t *rgb =
        WebPDecodeRGB(m_file.data(), m_file.size(), &width, &height);
    if (!rgb) throw ReadException("Cannot decode the WebP file " + filename());

    Frame tempFrame(size_t(width), size_t(height));
    Channel *r, *g, *b;
    tempFrame.createXYZChannels(r, g, b);
    const size_t size = size_t(width) * size_t(height);
    for (size_t i = 0; i < size; ++i) {
        (*r)(i) = rgb[i * 3] / 255.f;
        (*g)(i) = rgb[i * 3 + 1] / 255.f;
        (*b)(i) = rgb[i * 3 + 2] / 255.f;
    }
    WebPFree(rgb);
    FrameReader::read(tempFrame, params);
    frame.swap(tempFrame);
}

}  // io
}  // pfs
