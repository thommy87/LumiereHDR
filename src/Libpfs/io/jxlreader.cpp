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

#include "jxlreader.h"

#include <cstdio>
#include <memory>

#include <jxl/decode.h>
#include <jxl/version.h>

#include <Libpfs/frame.h>
#include <Libpfs/io/hdrencoding.h>

namespace pfs {
namespace io {

namespace {
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

typedef std::unique_ptr<JxlDecoder, decltype(&JxlDecoderDestroy)> Decoder;

//! the colour encoding the file was made in; false if it has only an ICC
//! profile
bool originalEncoding(JxlDecoder *decoder, JxlColorEncoding *color) {
#if defined(JPEGXL_COMPUTE_NUMERIC_VERSION) && \
    JPEGXL_NUMERIC_VERSION >= JPEGXL_COMPUTE_NUMERIC_VERSION(0, 9, 0)
    return JxlDecoderGetColorAsEncodedProfile(
               decoder, JXL_COLOR_PROFILE_TARGET_ORIGINAL, color) ==
           JXL_DEC_SUCCESS;
#else
    return JxlDecoderGetColorAsEncodedProfile(
               decoder, nullptr, JXL_COLOR_PROFILE_TARGET_ORIGINAL, color) ==
           JXL_DEC_SUCCESS;
#endif
}
}  // namespace

JxlReader::JxlReader(const std::string &filename) : FrameReader(filename) {
    open();
}

void JxlReader::open() {
    m_file = readFile(filename());
    if (m_file.empty()) throw InvalidFile("Cannot read " + filename());
    Decoder decoder(JxlDecoderCreate(nullptr), &JxlDecoderDestroy);
    JxlBasicInfo info;
    if (!decoder ||
        JxlDecoderSubscribeEvents(decoder.get(), JXL_DEC_BASIC_INFO) !=
            JXL_DEC_SUCCESS ||
        JxlDecoderSetInput(decoder.get(), m_file.data(), m_file.size()) !=
            JXL_DEC_SUCCESS ||
        JxlDecoderProcessInput(decoder.get()) != JXL_DEC_BASIC_INFO ||
        JxlDecoderGetBasicInfo(decoder.get(), &info) != JXL_DEC_SUCCESS) {
        m_file.clear();
        throw InvalidFile("Cannot read the JPEG XL file " + filename());
    }
    setWidth(info.xsize);
    setHeight(info.ysize);
}

bool JxlReader::isOpen() const { return !m_file.empty(); }

void JxlReader::close() { m_file.clear(); }

void JxlReader::read(Frame &frame, const Params &params) {
    if (!isOpen()) open();
    Decoder decoder(JxlDecoderCreate(nullptr), &JxlDecoderDestroy);
    if (!decoder ||
        JxlDecoderSubscribeEvents(decoder.get(), JXL_DEC_BASIC_INFO |
                                                     JXL_DEC_COLOR_ENCODING |
                                                     JXL_DEC_FULL_IMAGE) !=
            JXL_DEC_SUCCESS ||
        JxlDecoderSetInput(decoder.get(), m_file.data(), m_file.size()) !=
            JXL_DEC_SUCCESS)
        throw ReadException("Cannot decode the JPEG XL file " + filename());

    JxlBasicInfo info = {};
    JxlColorEncoding original = {};
    bool known = false, pq = false, hlg = false, wide = false;
    const JxlPixelFormat format = {3, JXL_TYPE_FLOAT, JXL_NATIVE_ENDIAN, 0};
    std::vector<float> pixels;

    for (;;) {
        const JxlDecoderStatus status = JxlDecoderProcessInput(decoder.get());
        if (status == JXL_DEC_BASIC_INFO) {
            JxlDecoderGetBasicInfo(decoder.get(), &info);
        } else if (status == JXL_DEC_COLOR_ENCODING) {
            known = originalEncoding(decoder.get(), &original);
            pq = known && original.transfer_function == JXL_TRANSFER_FUNCTION_PQ;
            hlg = known && original.transfer_function == JXL_TRANSFER_FUNCTION_HLG;
            wide = known && original.primaries == JXL_PRIMARIES_2100;
            // lossy files can be decoded straight into what we need: linear
            // Rec. 709 for HDR, sRGB for photos
            if (!info.uses_original_profile) {
                JxlColorEncoding wanted;
                JxlColorEncodingSetToSRGB(&wanted, JXL_FALSE);
                if (pq || hlg) wanted.transfer_function = JXL_TRANSFER_FUNCTION_LINEAR;
                JxlDecoderSetPreferredColorProfile(decoder.get(), &wanted);
            }
        } else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
            size_t bytes = 0;
            JxlDecoderImageOutBufferSize(decoder.get(), &format, &bytes);
            pixels.resize(bytes / sizeof(float));
            JxlDecoderSetImageOutBuffer(decoder.get(), &format, pixels.data(),
                                        pixels.size() * sizeof(float));
        } else if (status == JXL_DEC_FULL_IMAGE || status == JXL_DEC_SUCCESS) {
            break;
        } else {
            throw ReadException("Cannot decode the JPEG XL file " + filename());
        }
    }
    if (pixels.size() < size_t(info.xsize) * info.ysize * 3)
        throw ReadException("Incomplete JPEG XL file " + filename());

    Frame tempFrame(info.xsize, info.ysize);
    Channel *r, *g, *b;
    tempFrame.createXYZChannels(r, g, b);
    const size_t size = size_t(info.xsize) * info.ysize;
    // linear output of an HDR file: 1.0 is the file's peak brightness
    const float peak = info.intensity_target > 0 ? info.intensity_target : 10000.f;
    for (size_t i = 0; i < size; ++i) {
        float c[3] = {pixels[i * 3], pixels[i * 3 + 1], pixels[i * 3 + 2]};
        if (pq || hlg) {
            if (info.uses_original_profile) {
                // still in the file's own curve and colours
                for (float &v : c)
                    v = pq ? hdr::pqDecode(v) / hdr::kReferenceWhite
                           : hdr::hlgDecode(v) * 1000.f / hdr::kReferenceWhite;
                if (wide) hdr::rec2020To709(c[0], c[1], c[2]);
            } else {
                for (float &v : c) v = v * peak / hdr::kReferenceWhite;
            }
            for (float &v : c) v = std::max(v, 0.f);
        } else {
            for (float &v : c) v = std::min(std::max(v, 0.f), 1.f);
        }
        (*r)(i) = c[0];
        (*g)(i) = c[1];
        (*b)(i) = c[2];
    }
    FrameReader::read(tempFrame, params);
    frame.swap(tempFrame);
}

}  // io
}  // pfs
