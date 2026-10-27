/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 *    Copyright (C) 2007 by Nicholas Phillips
 *  Copyright (C) 2013 Davide Anastasia
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 */

#include "mtb_alignment.h"

#include <iso646.h>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

#include <Common/global.h>
#include <Libpfs/array2d.h>
#include <Libpfs/colorspace/convert.h>
#include <Libpfs/colorspace/xyz.h>
#include <Libpfs/frame.h>
#include <Libpfs/manip/resize.h>
#include <Libpfs/manip/shift.h>
#include <Libpfs/utils/transform.h>

#include <Libpfs/io/jpegwriter.h>
#include "arch/math.h"

using namespace std;
using namespace pfs;

#ifndef NDEBUG
#define PRINT_DEBUG(str) std::cerr << "MTB: " << str << std::endl
#else
#define PRINT_DEBUG(str)
#endif

typedef Array2D<uint8_t> Array2D8u;
typedef Array2D<bool> Array2Db;

namespace libhdr {

long XORimages(const Array2Db &img1, const Array2Db &mask1,
               const Array2Db &img2, const Array2Db &mask2) {
    long err = 0;
    for (size_t i = 0; i < img1.getRows(); i++) {
        Array2Db::const_iterator p1 = img1.row_begin(i);
        Array2Db::const_iterator p2 = img2.row_begin(i);
        Array2Db::const_iterator m1 = mask1.row_begin(i);
        Array2Db::const_iterator m2 = mask2.row_begin(i);

        for (size_t j = 0; j < img1.getCols(); j++) {
            err += (long)((*p1++ xor *p2++) and *m1++ and *m2++);
        }
    }
    return err;
}

// setThreshold gets the data from the input image and creates the threshold
// and mask images.
// Those are bitmap (0,1 valued) with depth()=1
void setThreshold(const Array2D8u &in, const int threshold, const int noise,
                  Array2Db &threshold_out, Array2Db &mask_out) {
    assert(in.getCols() == threshold_out.getCols());
    assert(in.getRows() == threshold_out.getRows());
    assert(in.getCols() == mask_out.getCols());
    assert(in.getRows() == mask_out.getRows());

    for (size_t i = 0; i < in.getRows(); i++) {
        Array2D8u::const_iterator inp = in.row_begin(i);

        Array2Db::iterator outp = threshold_out.row_begin(i);
        Array2Db::iterator maskp = mask_out.row_begin(i);

        for (size_t j = 0; j < in.getCols(); j++) {
            *outp++ = *inp < threshold ? 0 : 1;
            *maskp++ =
                (*inp > (threshold - noise)) && (*inp < (threshold + noise))
                    ? 0
                    : 1;
            ++inp;
        }
    }
}

void getExpShift(const Array2D8u &img1, const int median1,
                 const Array2D8u &img2, const int median2, const int noise,
                 const int shift_bits, int &shift_x, int &shift_y,
                 long *error = nullptr) {
    assert(img1.getCols() == img2.getCols());
    assert(img1.getRows() == img2.getRows());

    int curr_x = 0;
    int curr_y = 0;

    if (shift_bits > 0) {
        Array2D8u img1small(img1.getCols() / 2, img1.getRows() / 2);
        Array2D8u img2small(img2.getCols() / 2, img2.getRows() / 2);

        pfs::resize(img1, img1small, BilinearInterp);
        pfs::resize(img2, img2small, BilinearInterp);

        getExpShift(img1small, median1, img2small, median2, noise,
                    shift_bits - 1, curr_x, curr_y);
        curr_x *= 2;
        curr_y *= 2;
    }

    Array2Db img1threshold(img1.getCols(), img1.getRows());
    Array2Db img1mask(img1.getCols(), img1.getRows());
    Array2Db img2threshold(img2.getCols(), img2.getRows());
    Array2Db img2mask(img2.getCols(), img2.getRows());

    setThreshold(img1, median1, noise, img1threshold, img1mask);
    setThreshold(img2, median2, noise, img2threshold, img2mask);

    Array2Db img2_shifted(img2.getCols(), img2.getRows());
    Array2Db img2mask_shifted(img2.getCols(), img2.getRows());

    long minerr = long(img1.size()) + 1;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int dx = curr_x + i;
            int dy = curr_y + j;

            pfs::shift(img2threshold, dx, dy, img2_shifted);
            pfs::shift(img2mask, dx, dy, img2mask_shifted);

            long err = XORimages(img1threshold, img1mask, img2_shifted,
                                 img2mask_shifted);

            if (err < minerr) {
                minerr = err;
                shift_x = dx;
                shift_y = dy;
            }
        }
    }

    if (error) *error = minerr;

    PRINT_DEBUG("getExpShift::Level " << shift_bits << " shift (" << shift_x
                                      << "," << shift_y << ")");
}

namespace {

const double kPi = 3.14159265358979323846;

//! rotation around the centre, bilinear; outside the source becomes 0
template <typename T>
void rotateArray(const Array2D<T> &in, double radians, Array2D<T> &out) {
    const long w = long(in.getCols()), h = long(in.getRows());
    out.resize(in.getCols(), in.getRows());
    const double c = std::cos(radians), s = std::sin(radians);
    const double cx = (w - 1) / 2.0, cy = (h - 1) / 2.0;
    for (long y = 0; y < h; ++y)
        for (long x = 0; x < w; ++x) {
            // where this output pixel comes from in the input
            const double dx = x - cx, dy = y - cy;
            const double sx = c * dx + s * dy + cx;
            const double sy = -s * dx + c * dy + cy;
            const long x0 = long(std::floor(sx)), y0 = long(std::floor(sy));
            if (x0 < 0 || y0 < 0 || x0 + 1 >= w || y0 + 1 >= h) {
                out(size_t(x), size_t(y)) = T(0);
                continue;
            }
            const double ax = sx - x0, ay = sy - y0;
            const double v =
                (1 - ay) * ((1 - ax) * in(size_t(x0), size_t(y0)) +
                            ax * in(size_t(x0 + 1), size_t(y0))) +
                ay * ((1 - ax) * in(size_t(x0), size_t(y0 + 1)) +
                      ax * in(size_t(x0 + 1), size_t(y0 + 1)));
            out(size_t(x), size_t(y)) = T(v);
        }
}

//! the central part of an image, without the corners a rotation empties
Array2D8u centre(const Array2D8u &in) {
    const size_t mx = in.getCols() / 8, my = in.getRows() / 8;
    Array2D8u out(in.getCols() - 2 * mx, in.getRows() - 2 * my);
    for (size_t y = 0; y < out.getRows(); ++y)
        for (size_t x = 0; x < out.getCols(); ++x)
            out(x, y) = in(x + mx, y + my);
    return out;
}

//! the reference and the image at a reduced size, for judging rotations
class RotationProbe {
   public:
    RotationProbe(const Array2D8u &ref, int refMedian, const Array2D8u &img,
                  int imgMedian, int noise, size_t maxSide)
        : m_refMedian(refMedian), m_imgMedian(imgMedian), m_noise(noise) {
        const size_t longest = std::max(ref.getCols(), ref.getRows());
        const double factor =
            longest > maxSide ? double(maxSide) / longest : 1.0;
        const size_t w = std::max<size_t>(16, size_t(ref.getCols() * factor));
        const size_t h = std::max<size_t>(16, size_t(ref.getRows() * factor));
        Array2D8u refSmall(w, h);
        m_img.resize(w, h);
        pfs::resize(ref, refSmall, BilinearInterp);
        pfs::resize(img, m_img, BilinearInterp);
        m_refCentre = centre(refSmall);
        m_bits = std::max(
            int(std::floor(std::log2(double(std::min(w, h))))) - 6, 0);
    }

    //! mismatch after rotating by \a degrees and the best shift
    long error(double degrees) const {
        Array2D8u rotated;
        rotateArray(m_img, degrees * kPi / 180.0, rotated);
        const Array2D8u rotatedCentre = centre(rotated);
        int sx = 0, sy = 0;
        long err = 0;
        getExpShift(m_refCentre, m_refMedian, rotatedCentre, m_imgMedian,
                    m_noise, m_bits, sx, sy, &err);
        return err;
    }

   private:
    Array2D8u m_refCentre, m_img;
    int m_refMedian, m_imgMedian, m_noise, m_bits = 0;
};

//! rotation (degrees) that best matches \a img to \a ref, or 0 unless a
//! rotation helps clearly. Differently exposed photos give a noisy
//! mismatch, which once let chance minima pass as rotations of up to 3
//! degrees; so the search stays within +-2 degrees, the winner has to beat
//! "no rotation" by 15 %, and a second look at twice the size has to agree.
double estimateRotation(const Array2D8u &ref, int refMedian,
                        const Array2D8u &img, int imgMedian, int noise) {
    const RotationProbe coarseProbe(ref, refMedian, img, imgMedian, noise, 512);
    const long none = coarseProbe.error(0.0);
    double best = 0.0;
    long bestErr = none;
    for (double d = -2.0; d <= 2.0001; d += 0.25) {
        const long e = coarseProbe.error(d);
        if (e < bestErr) {
            bestErr = e;
            best = d;
        }
    }
    const double coarse = best;
    for (double d = coarse - 0.2; d <= coarse + 0.2001; d += 0.05) {
        const long e = coarseProbe.error(d);
        if (e < bestErr) {
            bestErr = e;
            best = d;
        }
    }
    if (best == 0.0 || bestErr >= 0.85 * double(none)) return 0.0;

    // confirm on a larger copy before touching the photo
    const RotationProbe fineProbe(ref, refMedian, img, imgMedian, noise, 1024);
    if (fineProbe.error(best) >= 0.9 * double(fineProbe.error(0.0)))
        return 0.0;
    return best;
}

//! share of the pixels both bitmaps are sure about that differ, after
//! moving \a img by \a dx, \a dy (Array2D shift convention)
double mismatch(const Array2D8u &ref, int refMedian, const Array2D8u &img,
                int imgMedian, int noise, int dx, int dy) {
    Array2Db refBits(ref.getCols(), ref.getRows());
    Array2Db refMask(ref.getCols(), ref.getRows());
    Array2Db imgBits(img.getCols(), img.getRows());
    Array2Db imgMask(img.getCols(), img.getRows());
    setThreshold(ref, refMedian, noise, refBits, refMask);
    setThreshold(img, imgMedian, noise, imgBits, imgMask);
    Array2Db movedBits(img.getCols(), img.getRows());
    Array2Db movedMask(img.getCols(), img.getRows());
    pfs::shift(imgBits, dx, dy, movedBits);
    pfs::shift(imgMask, dx, dy, movedMask);
    long differ = 0, counted = 0;
    // iterators, as Array2D<bool> sits on a std::vector<bool>
    Array2Db::const_iterator rb = refBits.begin(), rm = refMask.begin();
    Array2Db::const_iterator mb = movedBits.begin(), mm = movedMask.begin();
    for (; rb != refBits.end(); ++rb, ++rm, ++mb, ++mm) {
        if (!*rm || !*mm) continue;
        ++counted;
        if (*rb != *mb) ++differ;
    }
    return counted ? double(differ) / counted : 1.0;
}

}  // namespace

void rotateFrame(pfs::Frame &frame, double degrees) {
    const double radians = degrees * kPi / 180.0;
    for (pfs::Channel *channel : frame.getChannels()) {
        Array2D<float> rotated;
        rotateArray<float>(*channel, radians, rotated);
        std::copy(rotated.begin(), rotated.end(), channel->begin());
    }
}

int getLum(const Frame &in, Array2D8u &out, double quantile) {
    assert(quantile >= 0.0);
    assert(quantile <= 1.0);

    Array2D8u tempOut(in.getWidth(), in.getHeight());

    const Channel *R;
    const Channel *G;
    const Channel *B;

    in.getXYZChannels(R, G, B);
    // convert to grayscale...
    utils::transform(R->begin(), R->end(), G->begin(), B->begin(),
                     tempOut.begin(), colorspace::ConvertRGB2Y());

    // build histogram
    vector<long> hist(256, 0);
    assert(hist.size() == 256u);

    for (Array2D8u::iterator it = tempOut.begin(); it != tempOut.end(); ++it) {
        ++hist[*it];
    }

    // find the quantile...
    size_t relativeQuantile = in.size() * quantile;
    size_t idx = 0;
    size_t cdf = 0;
    for (; idx < hist.size(); ++idx) {
        cdf += hist[idx];

        if (cdf >= relativeQuantile) break;
    }

    // return values...
    out.swap(tempOut);
    return idx;
}

void mtbalign(const pfs::Frame &image1, const pfs::Frame &image2,
              const double quantile, const int noise, const int shift_bits,
              int &shift_x, int &shift_y) {
    Array2D8u img1lum;
    Array2D8u img2lum;

    int median1 = getLum(image1, img1lum, quantile);
    int median2 = getLum(image2, img2lum, quantile);

    PRINT_DEBUG("align::medians, image 1: " << median1
                                            << ", image 2: " << median2);
    getExpShift(img1lum, median1, img2lum, median2, noise, shift_bits, shift_x,
                shift_y);

    PRINT_DEBUG("align::done, final shift is (" << shift_x << "," << shift_y
                                                << ")");
}

static const double quantile = 0.5;
static const int noise = 4;

AlignmentMargins mtb_alignment(std::vector<pfs::FramePtr> &framePtrList) {
    AlignmentMargins margins;
    if (framePtrList.size() <= 1) return margins;

    int width = framePtrList[0]->getWidth();
    int height = framePtrList[0]->getHeight();

    int shift_bits =
        std::max((int)floor(log2((double)std::min(width, height))) - 6, 0);
    PRINT_DEBUG("width=" << width << ", height=" << height
                         << ", shift_bits=" << shift_bits);

    // the middle photo stays in place; every other photo is aligned to its
    // neighbour towards the middle, which is already in place. Neighbours
    // differ least in exposure, so their bitmaps agree; the darkest and the
    // brightest photo compared with the middle one directly often do not,
    // and then got thrown off by dozens of pixels.
    const size_t count = framePtrList.size();
    // the photos from darkest to brightest (they may come in any order),
    // by their average brightness
    std::vector<double> brightness(count);
    for (size_t i = 0; i < count; ++i) {
        Array2D8u probe;
        getLum(*framePtrList[i], probe, quantile);
        double sum = 0;
        for (const uint8_t v : probe) sum += v;
        brightness[i] = probe.size() ? sum / probe.size() : 0;
    }
    std::vector<size_t> sorted(count);
    for (size_t i = 0; i < count; ++i) sorted[i] = i;
    std::stable_sort(sorted.begin(), sorted.end(), [&](size_t a, size_t b) {
        return brightness[a] < brightness[b];
    });
    // the anchor is the best exposed one, closest to mid grey; a series
    // with many dark photos has it off centre
    size_t middle = count / 2;
    for (size_t r = 0; r < count; ++r)
        if (std::fabs(brightness[sorted[r]] - 128) <
            std::fabs(brightness[sorted[middle]] - 128))
            middle = r;
    // from the anchor outwards, each photo next to an aligned one
    std::vector<std::pair<size_t, size_t>> steps;  // photo, neighbour
    for (size_t r = middle; r-- > 0;) steps.push_back({sorted[r], sorted[r + 1]});
    for (size_t r = middle + 1; r < count; ++r)
        steps.push_back({sorted[r], sorted[r - 1]});

    for (const auto &step : steps) {
        const size_t i = step.first, neighbour = step.second;
        Array2D8u referenceLum;
        const int referenceMedian =
            getLum(*framePtrList[neighbour], referenceLum, quantile);
        Array2D8u lum;
        int median = getLum(*framePtrList[i], lum, quantile);

        const double degrees =
            estimateRotation(referenceLum, referenceMedian, lum, median, noise);
        int turnX = 0, turnY = 0;  // corners a rotation leaves empty
        if (degrees != 0.0) {
            PRINT_DEBUG("Rotating image " << i << " by " << degrees);
            rotateFrame(*framePtrList[i], degrees);
            median = getLum(*framePtrList[i], lum, quantile);
            const double s = std::fabs(std::sin(degrees * kPi / 180.0));
            turnX = int(std::ceil(height / 2.0 * s)) + 1;
            turnY = int(std::ceil(width / 2.0 * s)) + 1;
        }

        int shiftX = 0, shiftY = 0;
        getExpShift(referenceLum, referenceMedian, lum, median, noise,
                    shift_bits, shiftX, shiftY);
        // keep the photo where it is unless the shift matches clearly better
        if ((shiftX || shiftY) &&
            mismatch(referenceLum, referenceMedian, lum, median, noise, shiftX,
                     shiftY) >
                0.95 * mismatch(referenceLum, referenceMedian, lum, median,
                               noise, 0, 0)) {
            PRINT_DEBUG("Shift for image " << i << " rejected");
            shiftX = shiftY = 0;
        }
        if (shiftX || shiftY) {
            PRINT_DEBUG("Shift for image " << i << " = (" << shiftX << ","
                                           << shiftY << ")");
            // getExpShift answers in the convention of the Array2D shift
            // (out(x) = in(x + dx)); the Frame shift moves content the
            // other way, so the sign flips here. Using it unflipped, as the
            // old code did, moved frames further away instead of back.
            FramePtr shiftedFrame(
                pfs::shift(*framePtrList[i], -shiftX, -shiftY));
            framePtrList[i]->swap(*shiftedFrame);
        }
        // out(x) = in(x + shiftX): a positive shift empties the right edge
        margins.left = std::max(margins.left, turnX + std::max(0, -shiftX));
        margins.right = std::max(margins.right, turnX + std::max(0, shiftX));
        margins.top = std::max(margins.top, turnY + std::max(0, -shiftY));
        margins.bottom = std::max(margins.bottom, turnY + std::max(0, shiftY));
    }
    return margins;
}
}
