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

#include "avifwriter.h"

#include <cmath>
#include <cstdio>
#include <memory>

#include <avif/avif.h>

#include <Libpfs/io/hdrencoding.h>
#include <Libpfs/io/rgb8buffer.h>

namespace pfs {
namespace io {

AvifWriter::AvifWriter(const std::string &filename) : FrameWriter(filename) {}

AvifWriter::AvifWriter() : FrameWriter() {}

bool AvifWriter::write(const pfs::Frame &frame, const pfs::Params &params) {
    Rgb8WriterParams p;
    p.parse(params);
    std::vector<uint8_t> pixels = toRgb8(frame, p);

    const bool lossless = p.quality_ >= 100;
    std::unique_ptr<avifImage, decltype(&avifImageDestroy)> image(
        avifImageCreate(frame.getWidth(), frame.getHeight(), 8,
                        AVIF_PIXEL_FORMAT_YUV444),
        &avifImageDestroy);
    if (!image) throw WriteException("AVIF: cannot create the image");
    image->colorPrimaries = AVIF_COLOR_PRIMARIES_BT709;
    image->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_SRGB;
    // lossless needs RGB stored as is; otherwise the usual YUV matrix
    image->matrixCoefficients = lossless ? AVIF_MATRIX_COEFFICIENTS_IDENTITY
                                         : AVIF_MATRIX_COEFFICIENTS_BT601;
    image->yuvRange = AVIF_RANGE_FULL;

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, image.get());
    rgb.format = AVIF_RGB_FORMAT_RGB;
    rgb.depth = 8;
    rgb.pixels = pixels.data();
    rgb.rowBytes = frame.getWidth() * 3;
    if (avifImageRGBToYUV(image.get(), &rgb) != AVIF_RESULT_OK)
        throw WriteException("AVIF: cannot convert the image");

    std::unique_ptr<avifEncoder, decltype(&avifEncoderDestroy)> encoder(
        avifEncoderCreate(), &avifEncoderDestroy);
    if (!encoder) throw WriteException("AVIF: cannot create the encoder");
    encoder->quality = lossless ? AVIF_QUALITY_LOSSLESS : int(p.quality_);
    encoder->speed = 6;
    encoder->maxThreads = 4;

    avifRWData output = AVIF_DATA_EMPTY;
    const avifResult result =
        avifEncoderWrite(encoder.get(), image.get(), &output);
    if (result != AVIF_RESULT_OK) {
        avifRWDataFree(&output);
        throw WriteException(std::string("AVIF: ") + avifResultToString(result));
    }
    m_fileSize = output.size;

    bool ok = true;
    if (!filename().empty()) {
        FILE *f = std::fopen(filename().c_str(), "wb");
        if (f == nullptr) {
            avifRWDataFree(&output);
            throw InvalidFile("Cannot open file " + filename());
        }
        ok = std::fwrite(output.data, 1, output.size, f) == output.size;
        ok = (std::fclose(f) == 0) && ok;
    }
    avifRWDataFree(&output);
    if (!ok) throw WriteException("AVIF: cannot write " + filename());
    return true;
}

namespace {
//! encodes \a image and writes it to \a filename
void encodeAvif(avifImage *image, int quality, const std::string &filename) {
    std::unique_ptr<avifEncoder, decltype(&avifEncoderDestroy)> encoder(
        avifEncoderCreate(), &avifEncoderDestroy);
    if (!encoder) throw WriteException("AVIF: cannot create the encoder");
    encoder->quality = quality;
    encoder->speed = 6;
    encoder->maxThreads = 4;

    avifRWData output = AVIF_DATA_EMPTY;
    const avifResult result = avifEncoderWrite(encoder.get(), image, &output);
    if (result != AVIF_RESULT_OK) {
        avifRWDataFree(&output);
        throw WriteException(std::string("AVIF: ") + avifResultToString(result));
    }
    FILE *f = std::fopen(filename.c_str(), "wb");
    if (f == nullptr) {
        avifRWDataFree(&output);
        throw InvalidFile("Cannot open file " + filename);
    }
    bool ok = std::fwrite(output.data, 1, output.size, f) == output.size;
    ok = (std::fclose(f) == 0) && ok;
    avifRWDataFree(&output);
    if (!ok) throw WriteException("AVIF: cannot write " + filename);
}
}  // namespace

AvifHdrWriter::AvifHdrWriter(const std::string &filename)
    : FrameWriter(filename) {}

bool AvifHdrWriter::write(const pfs::Frame &frame, const pfs::Params &params) {
    Rgb8WriterParams p;
    p.parse(params);
    const size_t width = frame.getWidth(), height = frame.getHeight();
    const std::vector<float> nits = hdr::toNits2020(frame);

    // 10-bit PQ samples
    std::vector<uint16_t> pixels(nits.size());
    for (size_t i = 0; i < nits.size(); ++i)
        pixels[i] = uint16_t(std::lround(hdr::pqEncode(nits[i]) * 1023.f));

    std::unique_ptr<avifImage, decltype(&avifImageDestroy)> image(
        avifImageCreate(uint32_t(width), uint32_t(height), 10,
                        AVIF_PIXEL_FORMAT_YUV444),
        &avifImageDestroy);
    if (!image) throw WriteException("AVIF: cannot create the image");
    image->colorPrimaries = AVIF_COLOR_PRIMARIES_BT2020;
    image->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_SMPTE2084;
    image->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_BT2020_NCL;
    image->yuvRange = AVIF_RANGE_FULL;
    image->clli.maxCLL = uint16_t(hdr::kPeak);
    image->clli.maxPALL = uint16_t(hdr::kReferenceWhite);

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, image.get());
    rgb.format = AVIF_RGB_FORMAT_RGB;
    rgb.depth = 10;
    rgb.pixels = reinterpret_cast<uint8_t *>(pixels.data());
    rgb.rowBytes = uint32_t(width * 3 * sizeof(uint16_t));
    if (avifImageRGBToYUV(image.get(), &rgb) != AVIF_RESULT_OK)
        throw WriteException("AVIF: cannot convert the image");

    encodeAvif(image.get(),
               p.quality_ >= 100 ? AVIF_QUALITY_LOSSLESS : int(p.quality_),
               filename());
    return true;
}

}  // io
}  // pfs
