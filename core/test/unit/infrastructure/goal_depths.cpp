// goal_depths: per-goal depth map. Invariants: at-based access, duplicate set and
// invalid erase assert in debug, clear removes all entries.

#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/goal_depths.hpp"
#include "value_objects/lineage.hpp"

struct GoalDepthsTest : public ::testing::Test {
    goal_depths store;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};

    static constexpr size_t kDepth0 = 0;
    static constexpr size_t kDepth1 = 3;
};

TEST_F(GoalDepthsTest, GetOnUnknownGoalThrows) {
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(GoalDepthsTest, SetThenGetReturnsDepth) {
    store.set(&gl0, kDepth0);
    EXPECT_EQ(store.get(&gl0), kDepth0);
}

TEST_F(GoalDepthsTest, SecondSetThrows) {
    store.set(&gl0, kDepth0);
    EXPECT_THROW(store.set(&gl0, kDepth1), std::logic_error);
}

TEST_F(GoalDepthsTest, EraseRemovesEntry) {
    store.set(&gl0, kDepth0);
    store.erase(&gl0);
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(GoalDepthsTest, EraseOnUnknownGoalThrows) {
    EXPECT_THROW(store.erase(&gl0), std::logic_error);
}

TEST_F(GoalDepthsTest, EraseDoesNotAffectOtherGoals) {
    store.set(&gl0, kDepth0);
    store.set(&gl1, kDepth1);
    store.erase(&gl0);
    EXPECT_EQ(store.get(&gl1), kDepth1);
}

TEST_F(GoalDepthsTest, ClearGoalDepthsRemovesAllBindings) {
    store.set(&gl0, kDepth0);
    store.set(&gl1, kDepth1);
    store.clear_goal_depths();
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
    EXPECT_THROW(store.get(&gl1), std::out_of_range);
}
