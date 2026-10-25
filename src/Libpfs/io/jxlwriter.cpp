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

#include "jxlwriter.h"

#include <cstdio>
#include <memory>

#include <jxl/encode.h>
#include <jxl/thread_parallel_runner.h>

#include <Libpfs/io/rgb8buffer.h>

namespace pfs {
namespace io {

namespace {
// libjxl's own mapping from a JPEG-like quality to its "distance"
// (JxlEncoderDistanceFromQuality, which older libjxl versions lack)
float distanceFromQuality(float quality) {
    if (quality >= 100.f) return 0.f;
    if (quality >= 30.f) return 0.1f + (100.f - quality) * 0.09f;
    return 53.f / 3000.f * quality * quality - 23.f / 20.f * quality + 25.f;
}
}  // namespace

JxlWriter::JxlWriter(const std::string &filename) : FrameWriter(filename) {}

JxlWriter::JxlWriter() : FrameWriter() {}

bool JxlWriter::write(const pfs::Frame &frame, const pfs::Params &params) {
    Rgb8WriterParams p;
    p.parse(params);
    std::vector<uint8_t> pixels = toRgb8(frame, p);
    const bool lossless = p.quality_ >= 100;

    std::unique_ptr<JxlEncoder, decltype(&JxlEncoderDestroy)> encoder(
        JxlEncoderCreate(nullptr), &JxlEncoderDestroy);
    std::unique_ptr<void, decltype(&JxlThreadParallelRunnerDestroy)> runner(
        JxlThreadParallelRunnerCreate(
            nullptr, JxlThreadParallelRunnerDefaultNumWorkerThreads()),
        &JxlThreadParallelRunnerDestroy);
    if (!encoder || !runner)
        throw WriteException("JPEG XL: cannot create the encoder");
    if (JxlEncoderSetParallelRunner(encoder.get(), JxlThreadParallelRunner,
                                    runner.get()) != JXL_ENC_SUCCESS)
        throw WriteException("JPEG XL: cannot set up threads");

    JxlBasicInfo info;
    JxlEncoderInitBasicInfo(&info);
    info.xsize = frame.getWidth();
    info.ysize = frame.getHeight();
    info.bits_per_sample = 8;
    info.num_color_channels = 3;
    info.uses_original_profile = lossless ? JXL_TRUE : JXL_FALSE;
    if (JxlEncoderSetBasicInfo(encoder.get(), &info) != JXL_ENC_SUCCESS)
        throw WriteException("JPEG XL: invalid image size");

    JxlColorEncoding color;
    JxlColorEncodingSetToSRGB(&color, JXL_FALSE);
    if (JxlEncoderSetColorEncoding(encoder.get(), &color) != JXL_ENC_SUCCESS)
        throw WriteException("JPEG XL: cannot set sRGB");

    JxlEncoderFrameSettings *settings =
        JxlEncoderFrameSettingsCreate(encoder.get(), nullptr);
    if (lossless) {
        JxlEncoderSetFrameLossless(settings, JXL_TRUE);
    } else {
        JxlEncoderSetFrameDistance(settings,
                                   distanceFromQuality(float(p.quality_)));
    }

    const JxlPixelFormat format = {3, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};
    if (JxlEncoderAddImageFrame(settings, &format, pixels.data(),
                                pixels.size()) != JXL_ENC_SUCCESS)
        throw WriteException("JPEG XL: cannot add the image");
    JxlEncoderCloseInput(encoder.get());

    std::vector<uint8_t> output(64 * 1024);
    uint8_t *next = output.data();
    size_t avail = output.size();
    JxlEncoderStatus status;
    while ((status = JxlEncoderProcessOutput(encoder.get(), &next, &avail)) ==
           JXL_ENC_NEED_MORE_OUTPUT) {
        const size_t used = next - output.data();
        output.resize(output.size() * 2);
        next = output.data() + used;
        avail = output.size() - used;
    }
    if (status != JXL_ENC_SUCCESS)
        throw WriteException("JPEG XL: encoding failed");
    output.resize(next - output.data());
    m_fileSize = output.size();

    if (!filename().empty()) {
        FILE *f = std::fopen(filename().c_str(), "wb");
        if (f == nullptr) throw InvalidFile("Cannot open file " + filename());
        bool ok = std::fwrite(output.data(), 1, output.size(), f) ==
                  output.size();
        ok = (std::fclose(f) == 0) && ok;
        if (!ok) throw WriteException("JPEG XL: cannot write " + filename());
    }
    return true;
}

}  // io
}  // pfs
