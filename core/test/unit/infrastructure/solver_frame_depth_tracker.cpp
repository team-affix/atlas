// solver_frame_depth_tracker: integer counter for camped solver frames.

#include <gtest/gtest.h>
#include "infrastructure/solver_frame_depth_tracker.hpp"

struct SolverFrameDepthTrackerTest : public ::testing::Test {
    solver_frame_depth_tracker tracker;
};

TEST_F(SolverFrameDepthTrackerTest, StartsAtOne) {
    EXPECT_EQ(tracker.solver_frame_depth(), 1u);
}

TEST_F(SolverFrameDepthTrackerTest, PushIncrements) {
    tracker.push_frame();
    EXPECT_EQ(tracker.solver_frame_depth(), 2u);
    tracker.push_frame();
    EXPECT_EQ(tracker.solver_frame_depth(), 3u);
}

TEST_F(SolverFrameDepthTrackerTest, PopDecrements) {
    tracker.push_frame();
    tracker.push_frame();
    tracker.pop_frame();
    EXPECT_EQ(tracker.solver_frame_depth(), 2u);
    tracker.pop_frame();
    EXPECT_EQ(tracker.solver_frame_depth(), 1u);
}
