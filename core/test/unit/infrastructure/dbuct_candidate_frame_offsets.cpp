// dbuct_candidate_frame_offsets: set/get/unset and frame undo.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_candidate_frame_offsets.hpp"
#include "value_objects/lineage.hpp"

struct DbuctCandidateFrameOffsetsTest : public ::testing::Test {
    dbuct_candidate_frame_offsets offsets;
    resolution_lineage rl{nullptr, 0};
};

TEST_F(DbuctCandidateFrameOffsetsTest, SetGetRoundTrip) {
    offsets.push_frame();
    offsets.set(&rl, 42);
    EXPECT_EQ(offsets.get(&rl), 42u);
}

TEST_F(DbuctCandidateFrameOffsetsTest, UnsetRemovesOffset) {
    offsets.push_frame();
    offsets.set(&rl, 42);
    offsets.unset(&rl);
    EXPECT_THROW(offsets.get(&rl), std::out_of_range);
}

TEST_F(DbuctCandidateFrameOffsetsTest, PopFrameUndoesSet) {
    offsets.push_frame();
    offsets.push_frame();
    offsets.set(&rl, 7);
    offsets.pop_frame();
    EXPECT_THROW(offsets.get(&rl), std::out_of_range);
}
