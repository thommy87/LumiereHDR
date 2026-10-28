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

//! \brief Splits a folder of photos into exposure brackets

#ifndef LIBHDR_BRACKETGROUPING_H
#define LIBHDR_BRACKETGROUPING_H

#include <cstddef>
#include <vector>

namespace libhdr {

//! what is known about one photo; negative values mean unknown
struct Shot {
    double captureTime = -1;  //!< seconds, from the camera's clock
    double duration = -1;     //!< exposure time in seconds
    double exposure = 0;      //!< brightness in stops (log2 of the light let
                              //!< in), only compared within a bracket
    bool hasExposure = false;
};

struct BracketRules {
    //! longest pause between the end of one shot and the start of the next
    //! within a bracket, in seconds
    double maxPause = 3.0;
    //! two shots this close in brightness (stops) are not the same bracket
    double sameExposure = 0.2;
};

//! Groups \a shots (in the order given, which should be the order taken)
//! into brackets: a new bracket starts after a pause longer than
//! \a rules.maxPause or when a brightness repeats one already in the
//! bracket. Returns the indices of each bracket, or nothing when the shots
//! carry neither capture times nor exposures.
std::vector<std::vector<size_t>> groupBrackets(
    const std::vector<Shot> &shots, const BracketRules &rules = BracketRules());

}  // namespace libhdr

#endif  // LIBHDR_BRACKETGROUPING_H
