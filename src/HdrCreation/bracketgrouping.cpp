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

#include <HdrCreation/bracketgrouping.h>

#include <algorithm>
#include <cmath>

namespace libhdr {

std::vector<std::vector<size_t>> groupBrackets(const std::vector<Shot> &shots,
                                               const BracketRules &rules) {
    std::vector<std::vector<size_t>> brackets;
    bool anyTime = false, anyExposure = false;
    for (const Shot &shot : shots) {
        anyTime |= shot.captureTime >= 0;
        anyExposure |= shot.hasExposure;
    }
    if (!anyTime && !anyExposure) return brackets;

    for (size_t i = 0; i < shots.size(); ++i) {
        const Shot &shot = shots[i];
        bool startNew = brackets.empty();
        if (!startNew) {
            const Shot &previous = shots[brackets.back().back()];
            // a pause: the camera was not shooting a bracket any more
            if (shot.captureTime >= 0 && previous.captureTime >= 0) {
                const double end = previous.captureTime +
                                   std::max(previous.duration, 0.0);
                if (shot.captureTime - end > rules.maxPause) startNew = true;
            }
            // the same brightness again: the next bracket has begun
            if (!startNew && shot.hasExposure) {
                for (size_t j : brackets.back()) {
                    if (shots[j].hasExposure &&
                        std::fabs(shots[j].exposure - shot.exposure) <
                            rules.sameExposure) {
                        startNew = true;
                        break;
                    }
                }
            }
        }
        if (startNew) brackets.emplace_back();
        brackets.back().push_back(i);
    }
    return brackets;
}

}  // namespace libhdr
