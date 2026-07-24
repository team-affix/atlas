// goal_work_values: per-goal work map. Invariants: at-based access, duplicate set and
// invalid erase assert in debug, clear removes all entries.

#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/goal_work_values.hpp"
#include "value_objects/lineage.hpp"

struct GoalWorkValuesTest : public ::testing::Test {
    goal_work_values store;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};

    static constexpr double kWork0 = 0.5;
    static constexpr double kWork1 = 0.25;
};

TEST_F(GoalWorkValuesTest, GetOnUnknownGoalThrows) {
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(GoalWorkValuesTest, SetThenGetReturnsWork) {
    store.set(&gl0, kWork0);
    EXPECT_DOUBLE_EQ(store.get(&gl0), kWork0);
}

TEST_F(GoalWorkValuesTest, SecondSetThrows) {
    store.set(&gl0, kWork0);
    EXPECT_THROW(store.set(&gl0, kWork1), std::logic_error);
}

TEST_F(GoalWorkValuesTest, EraseRemovesEntry) {
    store.set(&gl0, kWork0);
    store.erase(&gl0);
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(GoalWorkValuesTest, EraseOnUnknownGoalThrows) {
    EXPECT_THROW(store.erase(&gl0), std::logic_error);
}

TEST_F(GoalWorkValuesTest, EraseDoesNotAffectOtherGoals) {
    store.set(&gl0, kWork0);
    store.set(&gl1, kWork1);
    store.erase(&gl0);
    EXPECT_DOUBLE_EQ(store.get(&gl1), kWork1);
}

TEST_F(GoalWorkValuesTest, ClearGoalWorkValuesRemovesAllBindings) {
    store.set(&gl0, kWork0);
    store.set(&gl1, kWork1);
    store.clear_goal_work_values();
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
    EXPECT_THROW(store.get(&gl1), std::out_of_range);
}
