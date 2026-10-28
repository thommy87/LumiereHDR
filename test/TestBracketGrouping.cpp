#include <gtest/gtest.h>

#include <HdrCreation/bracketgrouping.h>

using libhdr::Shot;
using libhdr::groupBrackets;

namespace {
Shot shot(double time, double duration, double exposure) {
    Shot s;
    s.captureTime = time;
    s.duration = duration;
    s.exposure = exposure;
    s.hasExposure = true;
    return s;
}

std::vector<size_t> sizes(const std::vector<std::vector<size_t>> &groups) {
    std::vector<size_t> out;
    for (const auto &g : groups) out.push_back(g.size());
    return out;
}
}  // namespace

TEST(TestBracketGrouping, BackToBackBracketsSplitWhereTheExposureRepeats) {
    // two brackets of three, shot right after each other
    std::vector<Shot> shots = {shot(0.0, 0.01, 0), shot(0.3, 0.0025, -2),
                               shot(0.6, 0.04, 2),  shot(1.0, 0.01, 0),
                               shot(1.3, 0.0025, -2), shot(1.6, 0.04, 2)};
    EXPECT_EQ(sizes(groupBrackets(shots)), (std::vector<size_t>{3, 3}));
}

TEST(TestBracketGrouping, APauseStartsANewBracket) {
    // a bracket of five, a walk to the next spot, a bracket of three
    std::vector<Shot> shots = {shot(0, 0.01, 0),    shot(0.4, 0.01, -1),
                               shot(0.8, 0.01, -2), shot(1.2, 0.01, 1),
                               shot(1.6, 0.01, 2),  shot(40, 0.01, 0.5),
                               shot(40.3, 0.01, -1.5), shot(40.6, 0.01, 2.5)};
    EXPECT_EQ(sizes(groupBrackets(shots)), (std::vector<size_t>{5, 3}));
}

TEST(TestBracketGrouping, ALongExposureIsNotAPause) {
    // the third shot starts 0.3 s after the second one ended, which itself
    // took 8 seconds
    std::vector<Shot> shots = {shot(0, 0.5, 0), shot(0.8, 8, 4),
                               shot(9.1, 2, 2)};
    EXPECT_EQ(sizes(groupBrackets(shots)), (std::vector<size_t>{3}));
}

TEST(TestBracketGrouping, WithoutTimesTheExposuresDecide) {
    std::vector<Shot> shots = {shot(-1, -1, 0), shot(-1, -1, -2),
                               shot(-1, -1, 2), shot(-1, -1, 0),
                               shot(-1, -1, -2)};
    EXPECT_EQ(sizes(groupBrackets(shots)), (std::vector<size_t>{3, 2}));
}

TEST(TestBracketGrouping, NothingKnownGivesNoGroups) {
    std::vector<Shot> shots(4);
    EXPECT_TRUE(groupBrackets(shots).empty());
}

TEST(TestBracketGrouping, PhotosWithoutInformationStayAlone) {
    // a bracket, a screenshot without EXIF, another bracket
    std::vector<Shot> shots = {shot(0, 0.01, 0), shot(0.3, 0.01, -2),
                               shot(0.6, 0.01, 2), Shot(),
                               shot(0.9, 0.01, 0.5), shot(1.2, 0.01, -1.5)};
    EXPECT_EQ(sizes(groupBrackets(shots)), (std::vector<size_t>{3, 1, 2}));
}
