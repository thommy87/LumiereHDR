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

#include <cstdio>
#include <jpeglib.h>

#include "ultrahdrwriter.h"

#include <algorithm>
#include <cmath>
#include <csetjmp>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include <ultrahdr_api.h>

#include <Libpfs/frame.h>
#include <Libpfs/io/boostmap.h>
#include <Libpfs/io/rgb8buffer.h>
#include <Libpfs/manip/resize.h>

namespace pfs {
namespace io {

namespace {


void check(const uhdr_error_info_t &status, const char *what) {
    if (status.error_code != UHDR_CODEC_OK) {
        std::string message = std::string("Ultra HDR: ") + what;
        if (status.has_detail) message += std::string(": ") + status.detail;
        throw WriteException(message);
    }
}

// libjpeg reports errors through longjmp; never let it call exit()
struct JpegError {
    jpeg_error_mgr manager;
    std::jmp_buf jump;
};

void jpegErrorExit(j_common_ptr cinfo) {
    std::longjmp(reinterpret_cast<JpegError *>(cinfo->err)->jump, 1);
}

//! RGB (3 bytes per pixel) or grey (1 byte) to a JPEG in memory
std::vector<uint8_t> encodeJpeg(const uint8_t *pixels, int width, int height,
                                bool grey, int quality) {
    jpeg_compress_struct cinfo;
    JpegError error;
    cinfo.err = jpeg_std_error(&error.manager);
    error.manager.error_exit = jpegErrorExit;
    unsigned char *buffer = nullptr;
    unsigned long size = 0;
    if (setjmp(error.jump)) {
        jpeg_destroy_compress(&cinfo);
        std::free(buffer);
        throw WriteException("Ultra HDR: JPEG encoding failed");
    }
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &buffer, &size);
    cinfo.image_width = JDIMENSION(width);
    cinfo.image_height = JDIMENSION(height);
    cinfo.input_components = grey ? 1 : 3;
    cinfo.in_color_space = grey ? JCS_GRAYSCALE : JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    const int stride = width * (grey ? 1 : 3);
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row = const_cast<JSAMPROW>(pixels) +
                       size_t(cinfo.next_scanline) * size_t(stride);
        jpeg_write_scanlines(&cinfo, &row, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    std::vector<uint8_t> out(buffer, buffer + size);
    std::free(buffer);
    return out;
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

    // the boost is measured on the HDR at the size of the result
    std::unique_ptr<pfs::Frame> resized;
    pfs::Frame *hdr = hdrSource.get();
    if (hdr->getWidth() != width || hdr->getHeight() != height) {
        resized.reset(pfs::resize(hdr, int(width), BilinearInterp));
        hdr = resized.get();
    }
    if (hdr->getWidth() != width || hdr->getHeight() != height)
        throw WriteException("Ultra HDR: HDR and result differ in shape");
    const BoostMap boost(*hdr, width, height);

    // The gain map is made here rather than by the library, which would
    // derive it pixel by pixel from the JPEG version of the result and so
    // bring edges and texture into it. One channel, a quarter of the size.
    const int scale = 4;
    const int mw = int(std::max<size_t>(1, (width + scale - 1) / scale));
    const int mh = int(std::max<size_t>(1, (height + scale - 1) / scale));
    std::vector<uint8_t> map(size_t(mw) * size_t(mh));
    const float logMax = std::log2(kMaxBoost);
    for (int y = 0; y < mh; ++y)
        for (int x = 0; x < mw; ++x) {
            const float b = boost.at((x + 0.5f) * scale - 0.5f,
                                     (y + 0.5f) * scale - 0.5f);
            const float recovery = std::log2(std::max(b, 1.f)) / logMax;
            map[size_t(y) * size_t(mw) + size_t(x)] = uint8_t(
                std::min(std::max(recovery * 255.f + 0.5f, 0.f), 255.f));
        }

    const int quality = int(std::min<size_t>(p.quality_, 100));
    std::vector<uint8_t> baseJpeg =
        encodeJpeg(rgb.data(), int(width), int(height), false, quality);
    std::vector<uint8_t> mapJpeg = encodeJpeg(map.data(), mw, mh, true, 90);

    std::unique_ptr<uhdr_codec_private_t, decltype(&uhdr_release_encoder)>
        encoder(uhdr_create_encoder(), &uhdr_release_encoder);
    if (!encoder) throw WriteException("Ultra HDR: cannot create the encoder");

    uhdr_compressed_image_t base = {};
    base.data = baseJpeg.data();
    base.data_sz = baseJpeg.size();
    base.capacity = baseJpeg.size();
    base.cg = UHDR_CG_BT_709;
    base.ct = UHDR_CT_SRGB;
    base.range = UHDR_CR_FULL_RANGE;
    check(uhdr_enc_set_compressed_image(encoder.get(), &base, UHDR_BASE_IMG),
          "result image");

    uhdr_compressed_image_t gainmap = {};
    gainmap.data = mapJpeg.data();
    gainmap.data_sz = mapJpeg.size();
    gainmap.capacity = mapJpeg.size();
    gainmap.cg = UHDR_CG_UNSPECIFIED;
    gainmap.ct = UHDR_CT_UNSPECIFIED;
    gainmap.range = UHDR_CR_FULL_RANGE;

    uhdr_gainmap_metadata_t metadata = {};
    for (int c = 0; c < 3; ++c) {
        metadata.max_content_boost[c] = kMaxBoost;
        metadata.min_content_boost[c] = 1.f;
        metadata.gamma[c] = 1.f;
        metadata.offset_sdr[c] = 1.f / 64.f;
        metadata.offset_hdr[c] = 1.f / 64.f;
    }
    metadata.hdr_capacity_min = 1.f;
    metadata.hdr_capacity_max = kMaxBoost;
    metadata.use_base_cg = 1;
    check(uhdr_enc_set_gainmap_image(encoder.get(), &gainmap, &metadata),
          "gain map");

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
