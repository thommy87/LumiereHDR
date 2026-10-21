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

// Regression tests for the Radiance (.hdr) reader: malformed run-length
// data must raise an exception instead of writing past the scanline.

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

#include "Libpfs/frame.h"
#include "Libpfs/io/ioexception.h"
#include "Libpfs/io/rgbereader.h"
#include "Libpfs/params.h"

namespace {

const int kWidth = 8;

std::string header(const std::string &size) {
    return "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n" + size + "\n";
}

// new-style RLE scanline marker for a scanline of kWidth pixels
std::string scanlineMarker() {
    return std::string("\x02\x02\x00", 3) + char(kWidth);
}

std::string writeFile(const std::string &name, const std::string &bytes) {
    const std::string path = "TestRGBEReader_" + name + ".hdr";
    std::ofstream out(path.c_str(), std::ios::binary);
    out.write(bytes.data(), bytes.size());
    return path;
}

void readFile(const std::string &path) {
    pfs::io::RGBEReader reader(path);
    pfs::Frame frame;
    reader.read(frame, pfs::Params());
}

}  // namespace

TEST(TestRGBEReader, ValidRunLengthScanline) {
    // each channel: one run of kWidth bytes (128 + kWidth, value)
    std::string data = header("-Y 1 +X 8") + scanlineMarker();
    data += std::string(1, char(128 + kWidth)) + char(100);  // R
    data += std::string(1, char(128 + kWidth)) + char(100);  // G
    data += std::string(1, char(128 + kWidth)) + char(100);  // B
    data += std::string(1, char(128 + kWidth)) + char(128);  // E

    const std::string path = writeFile("valid", data);
    EXPECT_NO_THROW(readFile(path));
    std::remove(path.c_str());
}

TEST(TestRGBEReader, RunLongerThanScanlineIsRejected) {
    // a run of 127 into an 8-pixel scanline used to overflow the buffer
    std::string data = header("-Y 1 +X 8") + scanlineMarker();
    for (int ch = 0; ch < 4; ++ch) data += std::string(1, char(255)) + char(1);

    const std::string path = writeFile("long_run", data);
    EXPECT_THROW(readFile(path), pfs::io::ReadException);
    std::remove(path.c_str());
}

TEST(TestRGBEReader, LiteralLongerThanScanlineIsRejected) {
    // a literal of 100 bytes into an 8-pixel scanline
    std::string data = header("-Y 1 +X 8") + scanlineMarker();
    data += std::string(1, char(100)) + std::string(200, char(7));

    const std::string path = writeFile("long_literal", data);
    EXPECT_THROW(readFile(path), pfs::io::ReadException);
    std::remove(path.c_str());
}

TEST(TestRGBEReader, InvalidSizesAreRejected) {
    const char *sizes[] = {"-Y 0 +X 8", "-Y 1 +X -5", "-Y 100000 +X 8",
                           "-Y 30000 +X 30000"};
    for (const char *size : sizes) {
        const std::string path = writeFile("size", header(size));
        EXPECT_THROW(pfs::io::RGBEReader reader(path), pfs::io::InvalidHeader)
            << size;
        std::remove(path.c_str());
    }
}

TEST(TestRGBEReader, TruncatedFileIsRejected) {
    const std::string path =
        writeFile("truncated", header("-Y 2 +X 8") + scanlineMarker());
    EXPECT_THROW(readFile(path), pfs::Exception);
    std::remove(path.c_str());
}

TEST(TestRGBEReader, HugeImageInTinyFileIsRejected) {
    // 30000 x 8000 pixels (240 MP, within the size limits) but only a few
    // bytes of data: must fail before allocating ~2.9 GB of channels
    const std::string path =
        writeFile("huge", header("-Y 8000 +X 30000") + scanlineMarker());
    EXPECT_THROW(readFile(path), pfs::io::ReadException);
    std::remove(path.c_str());
}
