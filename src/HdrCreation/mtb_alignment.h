/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 *  Copyright (C) 2007 Nicholas Phillips
 *  Copyright (C) 2013 Davide Anastasia
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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 */

//! \brief Implementation of the MTB algorithm for image alignment
//! \author Nicholas Phillips <ngphillips@gmail.com>
//! \author Giuseppe Rota (small modifications for Qt4)
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! Remove dependency from Qt and refactoring for new libhdr library

#ifndef LIBHDR_MTB_ALIGNMENT_H
#define LIBHDR_MTB_ALIGNMENT_H

#include <vector>

#include <Libpfs/array2d_fwd.h>
#include <Libpfs/frame.h>

namespace libhdr {

//! Aligns every frame to the middle one (the usual middle exposure):
//! finds a rotation of up to +-2 degrees and a shift for each, so that
//! hand-held brackets line up.
//! empty borders the alignment left on the photos, in pixels: there is
//! no picture there, and merged they turn into coloured blotches
struct AlignmentMargins {
    int left = 0, top = 0, right = 0, bottom = 0;
};

AlignmentMargins mtb_alignment(std::vector<pfs::FramePtr> &framePtrList);

//! Rotates all channels of \a frame around its centre, by \a degrees
//! (clockwise on screen for positive values); uncovered corners become 0.
void rotateFrame(pfs::Frame &frame, double degrees);

}  // libhdr

#endif
