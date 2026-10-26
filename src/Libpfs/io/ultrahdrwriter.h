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

//! \brief Writes Ultra HDR JPEGs (JPEG_R): the tone mapped frame as the
//! normal JPEG, plus a gain map from the HDR frame, which HDR screens use to
//! show the highlights brighter. The HDR frame comes in the parameter
//! "hdr_frame" (std::shared_ptr<pfs::Frame>).

#ifndef PFS_IO_ULTRAHDRWRITER_H
#define PFS_IO_ULTRAHDRWRITER_H

#include <string>

#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/ioexception.h>
#include <Libpfs/params.h>

namespace pfs {
namespace io {

class UltraHdrWriter : public FrameWriter {
   public:
    explicit UltraHdrWriter(const std::string &filename);

    bool write(const pfs::Frame &frame, const pfs::Params &params);

    size_t getFileSize() const { return m_fileSize; }

   private:
    size_t m_fileSize = 0;
};

}  // io
}  // pfs

#endif
