/**
 * This file is a part of LumiereHDR package.
 * ----------------------------------------------------------------------
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * ----------------------------------------------------------------------
 */

// libFuzzer target for the Radiance (.hdr) reader. The reader works on
// file names, so each input is written to a private temporary file.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <unistd.h>

#include "Libpfs/frame.h"
#include "Libpfs/io/rgbereader.h"
#include "Libpfs/params.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    char path[] = "/tmp/lumiere-fuzz-rgbe-XXXXXX";
    const int fd = mkstemp(path);
    if (fd < 0) return 0;
    const bool written = write(fd, data, size) == static_cast<ssize_t>(size);
    close(fd);

    if (written) {
        try {
            pfs::io::RGBEReader reader(path);
            pfs::Frame frame;
            reader.read(frame, pfs::Params());
        } catch (const std::exception &) {
            // rejecting malformed input is the expected outcome
        }
    }
    unlink(path);
    return 0;
}
