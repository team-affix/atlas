// candidate_frame_offsets: set/get/unset and clear_all.

#include <gtest/gtest.h>
#include "infrastructure/candidate_frame_offsets.hpp"
#include "value_objects/lineage.hpp"

struct CandidateFrameOffsetsTest : public ::testing::Test {
    candidate_frame_offsets offsets;
    resolution_lineage rl0{nullptr, 0};
    resolution_lineage rl1{nullptr, 1};
};

TEST_F(CandidateFrameOffsetsTest, SetGetRoundTrip) {
    offsets.set(&rl0, 42);
    EXPECT_EQ(offsets.get(&rl0), 42u);
}

TEST_F(CandidateFrameOffsetsTest, UnsetThenGetThrowsOutOfRange) {
    offsets.set(&rl0, 42);
    offsets.unset(&rl0);
    EXPECT_THROW(offsets.get(&rl0), std::out_of_range);
}

TEST_F(CandidateFrameOffsetsTest, ClearAllRemovesAllOffsets) {
    offsets.set(&rl0, 7);
    offsets.set(&rl1, 9);
    offsets.clear_candidate_frame_offsets();
    EXPECT_THROW(offsets.get(&rl0), std::out_of_range);
    EXPECT_THROW(offsets.get(&rl1), std::out_of_range);
}
