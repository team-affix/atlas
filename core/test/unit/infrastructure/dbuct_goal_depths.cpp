// dbuct_goal_depths: framed goal-depth map with undo on pop_frame.

#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/dbuct_goal_depths.hpp"
#include "value_objects/lineage.hpp"

struct DbuctGoalDepthsTest : public ::testing::Test {
    dbuct_goal_depths store;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};

    static constexpr size_t kDepth0 = 0;
    static constexpr size_t kDepth1 = 3;
};

TEST_F(DbuctGoalDepthsTest, GetOnUnknownGoalThrows) {
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(DbuctGoalDepthsTest, SetThenGetReturnsDepth) {
    store.set(&gl0, kDepth0);
    EXPECT_EQ(store.get(&gl0), kDepth0);
}

TEST_F(DbuctGoalDepthsTest, SecondSetThrows) {
    // A silently accepted duplicate would log an insert action whose undo erases
    // the entry the FIRST set created, losing a live goal's depth on pop.
    store.set(&gl0, kDepth0);
    EXPECT_THROW(store.set(&gl0, kDepth1), std::logic_error);
}

TEST_F(DbuctGoalDepthsTest, EraseOnUnknownGoalThrows) {
    EXPECT_THROW(store.erase(&gl0), std::logic_error);
}

TEST_F(DbuctGoalDepthsTest, PopFrameUndoesSet) {
    store.push_frame();
    store.set(&gl0, kDepth0);
    store.pop_frame();
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(DbuctGoalDepthsTest, PopFrameUndoesErase) {
    store.set(&gl0, kDepth0);
    store.push_frame();
    store.erase(&gl0);
    store.pop_frame();
    EXPECT_EQ(store.get(&gl0), kDepth0);
}

TEST_F(DbuctGoalDepthsTest, NestedFramesRestoreIndependently) {
    store.set(&gl0, kDepth0);
    store.push_frame();
    store.set(&gl1, kDepth1);
    store.push_frame();
    store.erase(&gl0);
    store.pop_frame();
    EXPECT_EQ(store.get(&gl0), kDepth0);
    EXPECT_EQ(store.get(&gl1), kDepth1);
    store.pop_frame();
    EXPECT_EQ(store.get(&gl0), kDepth0);
    EXPECT_THROW(store.get(&gl1), std::out_of_range);
}

TEST_F(DbuctGoalDepthsTest, MutationsInBaseFrameSurvivePushPopCycle) {
    // A pop must only replay the actions logged since its matching push.
    store.set(&gl0, kDepth0);
    store.set(&gl1, kDepth1);
    store.erase(&gl1);

    store.push_frame();
    store.pop_frame();

    EXPECT_EQ(store.get(&gl0), kDepth0);
    EXPECT_THROW(store.get(&gl1), std::out_of_range);
}
