/**
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

// AVIF, JPEG XL and WebP export: a small gradient must encode, lower quality
// must give a smaller file, and a file must reach the disk.

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

#include "Libpfs/frame.h"
#include "Libpfs/io/framewriterfactory.h"
#include "Libpfs/params.h"
#ifdef HAVE_LIBAVIF
#include "Libpfs/io/avifwriter.h"
#endif
#ifdef HAVE_LIBJXL
#include "Libpfs/io/jxlwriter.h"
#endif
#ifdef HAVE_LIBWEBP
#include "Libpfs/io/webpwriter.h"
#endif

namespace {

void fillGradient(pfs::Frame &frame) {
    pfs::Channel *r, *g, *b;
    frame.createXYZChannels(r, g, b);
    for (size_t y = 0; y < frame.getHeight(); ++y)
        for (size_t x = 0; x < frame.getWidth(); ++x) {
            (*r)(x, y) = float(x) / frame.getWidth();
            (*g)(x, y) = float(y) / frame.getHeight();
            (*b)(x, y) = float((x * 7 + y * 13) % 64) / 64.f;
        }
}

template <typename Writer>
void checkWriter(const std::string &extension) {
    pfs::Frame frame(96, 64);
    fillGradient(frame);

    Writer high;
    ASSERT_TRUE(high.write(frame, pfs::Params("quality", (size_t)95)));
    Writer low;
    ASSERT_TRUE(low.write(frame, pfs::Params("quality", (size_t)20)));
    EXPECT_GT(high.getFileSize(), 0u);
    EXPECT_LT(low.getFileSize(), high.getFileSize());

    // through the factory, by file extension, as the app saves
    const std::string path = "TestModernWriters." + extension;
    pfs::io::FrameWriterPtr writer =
        pfs::io::FrameWriterFactory::open(path, pfs::Params());
    ASSERT_TRUE(writer->write(frame, pfs::Params("quality", (size_t)80)));
    std::ifstream in(path.c_str(), std::ios::binary | std::ios::ate);
    EXPECT_GT(in.tellg(), 0);
    in.close();
    std::remove(path.c_str());
}

}  // namespace

#ifdef HAVE_LIBAVIF
TEST(TestModernWriters, Avif) { checkWriter<pfs::io::AvifWriter>("avif"); }
#endif

#ifdef HAVE_LIBJXL
TEST(TestModernWriters, JpegXl) { checkWriter<pfs::io::JxlWriter>("jxl"); }
#endif

#ifdef HAVE_LIBWEBP
TEST(TestModernWriters, WebP) { checkWriter<pfs::io::WebpWriter>("webp"); }
#endif
