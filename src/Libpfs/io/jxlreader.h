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

//! \brief Reads JPEG XL files: normal ones as photos (values 0..1, as a
//! JPEG is read), HDR ones (PQ or HLG) as linear HDR, normal white = 1

#ifndef PFS_IO_JXLREADER_H
#define PFS_IO_JXLREADER_H

#include <string>
#include <vector>

#include <Libpfs/io/framereader.h>
#include <Libpfs/io/ioexception.h>

namespace pfs {
namespace io {

class JxlReader : public FrameReader {
   public:
    explicit JxlReader(const std::string &filename);

    void open() override;
    bool isOpen() const override;
    void close() override;
    void read(Frame &frame, const Params &params) override;

   private:
    std::vector<uint8_t> m_file;
};

}  // io
}  // pfs

#endif
