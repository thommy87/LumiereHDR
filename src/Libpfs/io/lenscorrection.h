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

//! \brief Lens correction of RAW photos with the Lensfun database:
//! distortion, vignetting and colour fringes (lateral chromatic aberration)

#ifndef PFS_IO_LENSCORRECTION_H
#define PFS_IO_LENSCORRECTION_H

#include <string>

namespace pfs {
class Frame;

namespace io {

//! what the RAW file says about camera and lens
struct LensInfo {
    std::string make, model, lens;
    float focal = 0.f, aperture = 0.f, distance = 1000.f;
};

//! Corrects \a frame (linear RGB) in place. \a databaseDir: a folder of
//! Lensfun XML files, or empty for Lensfun's own places. Returns false and
//! leaves the frame alone when camera or lens are not in the database;
//! \a note says which lens was used, or why none.
bool correctLens(Frame &frame, const LensInfo &info,
                 const std::string &databaseDir, std::string *note = nullptr);

}  // io
}  // pfs

#endif
