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

#include "ultrahdrwriter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include <ultrahdr_api.h>

#include <Libpfs/frame.h>
#include <Libpfs/io/rgb8buffer.h>
#include <Libpfs/manip/resize.h>

namespace pfs {
namespace io {

namespace {

uint16_t toHalf(float value) {
    // IEEE 754 binary16 from binary32, rounding to nearest, no NaN input
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    const uint32_t sign = (bits >> 16) & 0x8000u;
    int32_t exponent = int32_t((bits >> 23) & 0xffu) - 127 + 15;
    uint32_t mantissa = bits & 0x7fffffu;
    if (exponent <= 0) {
        if (exponent < -10) return uint16_t(sign);
        mantissa |= 0x800000u;
        const int shift = 14 - exponent;
        uint32_t half = mantissa >> shift;
        if ((mantissa >> (shift - 1)) & 1u) ++half;
        return uint16_t(sign | half);
    }
    if (exponent >= 31) return uint16_t(sign | 0x7c00u);
    uint32_t half = sign | (uint32_t(exponent) << 10) | (mantissa >> 13);
    if (mantissa & 0x1000u) ++half;  // may carry into the exponent: fine
    return uint16_t(half);
}

float srgbToLinear(float v) {
    return v <= 0.04045f ? v / 12.92f
                         : std::pow((v + 0.055f) / 1.055f, 2.4f);
}

void check(const uhdr_error_info_t &status, const char *what) {
    if (status.error_code != UHDR_CODEC_OK) {
        std::string message = std::string("Ultra HDR: ") + what;
        if (status.has_detail) message += std::string(": ") + status.detail;
        throw WriteException(message);
    }
}

}  // namespace

UltraHdrWriter::UltraHdrWriter(const std::string &filename)
    : FrameWriter(filename) {}

bool UltraHdrWriter::write(const pfs::Frame &frame, const pfs::Params &params) {
    std::shared_ptr<pfs::Frame> hdrSource;
    if (!params.get("hdr_frame", hdrSource) || !hdrSource)
        throw WriteException("Ultra HDR: the HDR image is missing");

    Rgb8WriterParams p;
    p.parse(params);
    const size_t width = frame.getWidth();
    const size_t height = frame.getHeight();
    const std::vector<uint8_t> rgb = toRgb8(frame, p);

    // the gain map needs the HDR at the size of the tone mapped result
    std::unique_ptr<pfs::Frame> resized;
    pfs::Frame *hdr = hdrSource.get();
    if (hdr->getWidth() != width || hdr->getHeight() != height) {
        resized.reset(pfs::resize(hdr, int(width), BilinearInterp));
        hdr = resized.get();
    }
    if (hdr->getWidth() != width || hdr->getHeight() != height)
        throw WriteException("Ultra HDR: HDR and result differ in shape");
    const pfs::Channel *hr, *hg, *hb;
    hdr->getXYZChannels(hr, hg, hb);
    if (!hr || !hg || !hb)
        throw WriteException("Ultra HDR: the HDR image has no colour data");

    // HDR values are on an arbitrary scale; match them to the result in
    // the mid-tones so that 1.0 is the result's white (203 nits) and the
    // gain map only brightens what the HDR holds above it
    const size_t n = width * height;
    std::vector<float> ratios;
    ratios.reserve(n / 16 + 1);
    for (size_t i = 0; i < n; i += 16) {
        const float sdr = 0.2126f * srgbToLinear(rgb[3 * i] / 255.f) +
                          0.7152f * srgbToLinear(rgb[3 * i + 1] / 255.f) +
                          0.0722f * srgbToLinear(rgb[3 * i + 2] / 255.f);
        const float lum =
            0.2126f * (*hr)(i) + 0.7152f * (*hg)(i) + 0.0722f * (*hb)(i);
        if (sdr > 0.05f && sdr < 0.5f && lum > 0.f) ratios.push_back(sdr / lum);
    }
    float scale = 1.f;
    if (!ratios.empty()) {
        std::nth_element(ratios.begin(), ratios.begin() + ratios.size() / 2,
                         ratios.end());
        scale = ratios[ratios.size() / 2];
    }

    const float maxValue = 10000.f / 203.f;
    std::vector<uint16_t> half(n * 4);
    std::vector<uint8_t> rgba(n * 4);
    for (size_t i = 0; i < n; ++i) {
        const float values[3] = {(*hr)(i), (*hg)(i), (*hb)(i)};
        for (int c = 0; c < 3; ++c)
            half[4 * i + c] =
                toHalf(std::min(std::max(values[c] * scale, 0.f), maxValue));
        half[4 * i + 3] = toHalf(1.f);
        rgba[4 * i] = rgb[3 * i];
        rgba[4 * i + 1] = rgb[3 * i + 1];
        rgba[4 * i + 2] = rgb[3 * i + 2];
        rgba[4 * i + 3] = 255;
    }

    std::unique_ptr<uhdr_codec_private_t, decltype(&uhdr_release_encoder)>
        encoder(uhdr_create_encoder(), &uhdr_release_encoder);
    if (!encoder) throw WriteException("Ultra HDR: cannot create the encoder");

    uhdr_raw_image_t hdrImage = {};
    hdrImage.fmt = UHDR_IMG_FMT_64bppRGBAHalfFloat;
    hdrImage.cg = UHDR_CG_BT_709;
    hdrImage.ct = UHDR_CT_LINEAR;
    hdrImage.range = UHDR_CR_FULL_RANGE;
    hdrImage.w = unsigned(width);
    hdrImage.h = unsigned(height);
    hdrImage.planes[UHDR_PLANE_PACKED] = half.data();
    hdrImage.stride[UHDR_PLANE_PACKED] = unsigned(width);
    check(uhdr_enc_set_raw_image(encoder.get(), &hdrImage, UHDR_HDR_IMG),
          "HDR image");

    uhdr_raw_image_t sdrImage = {};
    sdrImage.fmt = UHDR_IMG_FMT_32bppRGBA8888;
    sdrImage.cg = UHDR_CG_BT_709;
    sdrImage.ct = UHDR_CT_SRGB;
    sdrImage.range = UHDR_CR_FULL_RANGE;
    sdrImage.w = unsigned(width);
    sdrImage.h = unsigned(height);
    sdrImage.planes[UHDR_PLANE_PACKED] = rgba.data();
    sdrImage.stride[UHDR_PLANE_PACKED] = unsigned(width);
    check(uhdr_enc_set_raw_image(encoder.get(), &sdrImage, UHDR_SDR_IMG),
          "result image");

    const int quality = int(std::min<size_t>(p.quality_, 100));
    check(uhdr_enc_set_quality(encoder.get(), quality, UHDR_BASE_IMG),
          "quality");
    check(uhdr_enc_set_quality(encoder.get(), std::min(quality, 95),
                               UHDR_GAIN_MAP_IMG),
          "gain map quality");
    // up to three stops above the result's white, shown on 1000-nit screens
    check(uhdr_enc_set_min_max_content_boost(encoder.get(), 1.f, 8.f),
          "content boost");
    check(uhdr_enc_set_target_display_peak_brightness(encoder.get(), 1000.f),
          "peak brightness");

    check(uhdr_encode(encoder.get()), "encoding");
    uhdr_compressed_image_t *out = uhdr_get_encoded_stream(encoder.get());
    if (out == nullptr || out->data == nullptr)
        throw WriteException("Ultra HDR: no output");
    m_fileSize = out->data_sz;

    FILE *f = std::fopen(filename().c_str(), "wb");
    if (f == nullptr) throw InvalidFile("Cannot open file " + filename());
    bool ok = std::fwrite(out->data, 1, out->data_sz, f) == out->data_sz;
    ok = (std::fclose(f) == 0) && ok;
    if (!ok) throw WriteException("Ultra HDR: cannot write " + filename());
    return true;
}

}  // io
}  // pfs
