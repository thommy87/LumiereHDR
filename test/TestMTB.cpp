#include <gtest/gtest.h>

#include <HdrCreation/mtb_alignment.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include <Libpfs/frame.h>
#include <Libpfs/manip/shift.h>

namespace {

// a deterministic scene with edges in every direction
pfs::FramePtr makeScene(size_t w, size_t h, float gain) {
    pfs::FramePtr frame = std::make_shared<pfs::Frame>(w, h);
    pfs::Channel *r, *g, *b;
    frame->createXYZChannels(r, g, b);
    for (size_t y = 0; y < h; ++y)
        for (size_t x = 0; x < w; ++x) {
            const double u = double(x) / w, v = double(y) / h;
            double value = 0.2 + 0.6 * ((int(x / 23) + int(y / 31)) % 2);
            value += 0.3 * std::sin(17 * u + 11 * v);
            if ((x - w / 3) * (x - w / 3) + (y - h / 2) * (y - h / 2) < 900)
                value = 1.2;
            const float f = float(std::max(value, 0.0)) * gain;
            (*r)(x, y) = f;
            (*g)(x, y) = 0.9f * f;
            (*b)(x, y) = 0.8f * f;
        }
    return frame;
}

// mean absolute difference over the central half, where nothing was cut
double centreDifference(const pfs::Frame &a, const pfs::Frame &b,
                        float gainA, float gainB) {
    const pfs::Channel *ra, *ga, *ba, *rb, *gb, *bb;
    a.getXYZChannels(ra, ga, ba);
    b.getXYZChannels(rb, gb, bb);
    double sum = 0;
    size_t n = 0;
    for (size_t y = a.getHeight() / 4; y < a.getHeight() * 3 / 4; ++y)
        for (size_t x = a.getWidth() / 4; x < a.getWidth() * 3 / 4; ++x) {
            sum += std::fabs((*ra)(x, y) / gainA - (*rb)(x, y) / gainB);
            ++n;
        }
    return sum / n;
}

}  // namespace

TEST(TestMTB, RotatedAndShiftedBracketIsAligned) {
    const size_t w = 480, h = 320;
    // three exposures; the darker and the brighter one were taken by hand
    pfs::FramePtr dark = makeScene(w, h, 0.5f);
    pfs::FramePtr middle = makeScene(w, h, 1.0f);
    pfs::FramePtr bright = makeScene(w, h, 2.0f);
    libhdr::rotateFrame(*dark, 1.5);
    pfs::FramePtr moved(pfs::shift(*dark, 7, -4));
    dark->swap(*moved);
    libhdr::rotateFrame(*bright, -1.0);

    const double before = centreDifference(*dark, *middle, 0.5f, 1.0f);
    std::vector<pfs::FramePtr> frames = {dark, middle, bright};
    libhdr::mtb_alignment(frames);
    const double after = centreDifference(*dark, *middle, 0.5f, 1.0f);
    const double afterBright = centreDifference(*bright, *middle, 2.0f, 1.0f);

    EXPECT_LT(after, before * 0.3);
    EXPECT_LT(after, 0.08);
    EXPECT_LT(afterBright, 0.08);
}

namespace {
//! like makeScene, but a camera clips what is too bright
pfs::FramePtr makeClippedScene(size_t w, size_t h, float gain) {
    pfs::FramePtr frame = makeScene(w, h, gain);
    for (pfs::Channel *channel : frame->getChannels())
        for (float &v : *channel) v = std::min(v, 1.0f);
    return frame;
}
}  // namespace

TEST(TestMTB, WideBracketOuterPhotosAreAligned) {
    const size_t w = 480, h = 320;
    // five photos two stops apart; the darkest and the brightest moved
    const float gains[] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
    std::vector<pfs::FramePtr> frames, originals;
    for (float g : gains) {
        frames.push_back(makeClippedScene(w, h, g));
        originals.push_back(makeClippedScene(w, h, g));
    }
    pfs::FramePtr darkMoved(pfs::shift(*frames[0], 6, -3));
    frames[0]->swap(*darkMoved);
    pfs::FramePtr brightMoved(pfs::shift(*frames[4], -5, 4));
    frames[4]->swap(*brightMoved);
    const double beforeDark = centreDifference(*frames[0], *originals[0], 1, 1);
    const double beforeBright =
        centreDifference(*frames[4], *originals[4], 1, 1);

    libhdr::mtb_alignment(frames);

    EXPECT_LT(centreDifference(*frames[0], *originals[0], 1, 1),
              beforeDark * 0.2);
    EXPECT_LT(centreDifference(*frames[4], *originals[4], 1, 1),
              beforeBright * 0.2);
    // the photos that were in place stay there
    for (size_t i = 1; i < 4; ++i)
        EXPECT_LT(centreDifference(*frames[i], *originals[i], 1, 1), 1e-6);
}
