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

//! \brief Writes 8-bit sRGB Jxl files. Without a file name the image
//! is only encoded in memory, to report the file size for a quality.

#ifndef PFS_IO_JXLWRITER_H
#define PFS_IO_JXLWRITER_H

#include <string>

#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/ioexception.h>
#include <Libpfs/params.h>

namespace pfs {
namespace io {

class JxlWriter : public FrameWriter {
   public:
    explicit JxlWriter(const std::string &filename);
    JxlWriter();

    bool write(const pfs::Frame &frame, const pfs::Params &params);

    size_t getFileSize() const { return m_fileSize; }

   private:
    size_t m_fileSize = 0;
};

}  // io
}  // pfs

#endif
