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

//! \brief Reads AVIF files: normal ones as photos (values 0..1, as a JPEG
//! is read), HDR ones (PQ or HLG) as linear HDR, normal white = 1

#ifndef PFS_IO_AVIFREADER_H
#define PFS_IO_AVIFREADER_H

#include <memory>
#include <string>

#include <Libpfs/io/framereader.h>
#include <Libpfs/io/ioexception.h>

namespace pfs {
namespace io {

class AvifReader : public FrameReader {
   public:
    explicit AvifReader(const std::string &filename);
    ~AvifReader() override;

    void open() override;
    bool isOpen() const override;
    void close() override;
    void read(Frame &frame, const Params &params) override;

   private:
    struct Data;
    std::unique_ptr<Data> m_data;
};

}  // io
}  // pfs

#endif
