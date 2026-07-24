// dbuct_goal_work_values: framed goal-work map with undo on pop_frame.

#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/dbuct_goal_work_values.hpp"
#include "value_objects/lineage.hpp"

struct DbuctGoalWorkValuesTest : public ::testing::Test {
    dbuct_goal_work_values store;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};

    static constexpr double kWork0 = 0.5;
    static constexpr double kWork1 = 0.25;
};

TEST_F(DbuctGoalWorkValuesTest, SetThenGetReturnsWork) {
    store.set(&gl0, kWork0);
    EXPECT_DOUBLE_EQ(store.get(&gl0), kWork0);
}

TEST_F(DbuctGoalWorkValuesTest, PopFrameUndoesSet) {
    store.push_frame();
    store.set(&gl0, kWork0);
    store.pop_frame();
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(DbuctGoalWorkValuesTest, PopFrameUndoesErase) {
    store.set(&gl0, kWork0);
    store.push_frame();
    store.erase(&gl0);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(&gl0), kWork0);
}

TEST_F(DbuctGoalWorkValuesTest, NestedFramesRestoreIndependently) {
    store.set(&gl0, kWork0);
    store.push_frame();
    store.set(&gl1, kWork1);
    store.push_frame();
    store.erase(&gl0);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(&gl0), kWork0);
    EXPECT_DOUBLE_EQ(store.get(&gl1), kWork1);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(&gl0), kWork0);
    EXPECT_THROW(store.get(&gl1), std::out_of_range);
}
