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

#include "avifreader.h"

#include <vector>

#include <avif/avif.h>

#include <Libpfs/frame.h>
#include <Libpfs/io/hdrencoding.h>

namespace pfs {
namespace io {

struct AvifReader::Data {
    avifDecoder *decoder = nullptr;
    ~Data() {
        if (decoder) avifDecoderDestroy(decoder);
    }
};

AvifReader::AvifReader(const std::string &filename)
    : FrameReader(filename) {
    open();
}

AvifReader::~AvifReader() { close(); }

void AvifReader::open() {
    m_data.reset(new Data);
    m_data->decoder = avifDecoderCreate();
    if (!m_data->decoder) throw InvalidFile("AVIF: cannot create the decoder");
    if (avifDecoderSetIOFile(m_data->decoder, filename().c_str()) !=
            AVIF_RESULT_OK ||
        avifDecoderParse(m_data->decoder) != AVIF_RESULT_OK) {
        close();
        throw InvalidFile("Cannot read the AVIF file " + filename());
    }
    setWidth(m_data->decoder->image->width);
    setHeight(m_data->decoder->image->height);
}

bool AvifReader::isOpen() const { return m_data && m_data->decoder; }

void AvifReader::close() { m_data.reset(); }

void AvifReader::read(Frame &frame, const Params &params) {
    if (!isOpen()) open();
    avifDecoder *decoder = m_data->decoder;
    if (avifDecoderNextImage(decoder) != AVIF_RESULT_OK)
        throw ReadException("Cannot decode the AVIF file " + filename());
    const avifImage *image = decoder->image;

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, image);
    rgb.format = AVIF_RGB_FORMAT_RGB;
    rgb.depth = image->depth > 8 ? 16 : 8;
    std::vector<uint8_t> buffer(size_t(image->width) * image->height * 3 *
                                (rgb.depth > 8 ? 2 : 1));
    rgb.pixels = buffer.data();
    rgb.rowBytes = image->width * 3 * (rgb.depth > 8 ? 2 : 1);
    if (avifImageYUVToRGB(image, &rgb) != AVIF_RESULT_OK)
        throw ReadException("Cannot convert the AVIF file " + filename());

    const bool pq =
        image->transferCharacteristics == AVIF_TRANSFER_CHARACTERISTICS_SMPTE2084;
    const bool hlg =
        image->transferCharacteristics == AVIF_TRANSFER_CHARACTERISTICS_HLG;
    const bool wide = image->colorPrimaries == AVIF_COLOR_PRIMARIES_BT2020;
    const float maxValue = rgb.depth > 8 ? 65535.f : 255.f;

    Frame tempFrame(image->width, image->height);
    Channel *r, *g, *b;
    tempFrame.createXYZChannels(r, g, b);
    const size_t size = size_t(image->width) * image->height;
    for (size_t i = 0; i < size; ++i) {
        float c[3];
        for (int k = 0; k < 3; ++k) {
            const float v =
                rgb.depth > 8
                    ? float(reinterpret_cast<const uint16_t *>(buffer.data())[i * 3 + k])
                    : float(buffer[i * 3 + k]);
            c[k] = v / maxValue;
        }
        if (pq || hlg) {
            // linear light, normal white = 1
            for (float &v : c)
                v = pq ? hdr::pqDecode(v) / hdr::kReferenceWhite
                       : hdr::hlgDecode(v) * 1000.f / hdr::kReferenceWhite;
            if (wide) hdr::rec2020To709(c[0], c[1], c[2]);
            for (float &v : c) v = std::max(v, 0.f);
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
