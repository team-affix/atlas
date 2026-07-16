// dbuct_goal_weights: framed goal-weight map with undo on pop_frame.

#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/dbuct_goal_weights.hpp"
#include "value_objects/lineage.hpp"

struct DbuctGoalWeightsTest : public ::testing::Test {
    dbuct_goal_weights store;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};

    static constexpr double kWeight0 = 0.5;
    static constexpr double kWeight1 = 0.25;
};

TEST_F(DbuctGoalWeightsTest, SetThenGetReturnsWeight) {
    store.set(&gl0, kWeight0);
    EXPECT_DOUBLE_EQ(store.get(&gl0), kWeight0);
}

TEST_F(DbuctGoalWeightsTest, PopFrameUndoesSet) {
    store.push_frame();
    store.set(&gl0, kWeight0);
    store.pop_frame();
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(DbuctGoalWeightsTest, PopFrameUndoesErase) {
    store.set(&gl0, kWeight0);
    store.push_frame();
    store.erase(&gl0);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(&gl0), kWeight0);
}

TEST_F(DbuctGoalWeightsTest, NestedFramesRestoreIndependently) {
    store.set(&gl0, kWeight0);
    store.push_frame();
    store.set(&gl1, kWeight1);
    store.push_frame();
    store.erase(&gl0);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(&gl0), kWeight0);
    EXPECT_DOUBLE_EQ(store.get(&gl1), kWeight1);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(&gl0), kWeight0);
    EXPECT_THROW(store.get(&gl1), std::out_of_range);
}
