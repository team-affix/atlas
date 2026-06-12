// goal_weights: per-goal weight map. Invariants: at-based access, duplicate set and
// invalid erase assert in debug, clear removes all entries.

#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/goal_weights.hpp"
#include "value_objects/lineage.hpp"

struct GoalWeightsTest : public ::testing::Test {
    goal_weights store;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};

    static constexpr double kWeight0 = 0.5;
    static constexpr double kWeight1 = 0.25;
};

TEST_F(GoalWeightsTest, GetOnUnknownGoalThrows) {
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(GoalWeightsTest, SetThenGetReturnsWeight) {
    store.set(&gl0, kWeight0);
    EXPECT_DOUBLE_EQ(store.get(&gl0), kWeight0);
}

TEST_F(GoalWeightsTest, SecondSetThrows) {
    store.set(&gl0, kWeight0);
    EXPECT_THROW(store.set(&gl0, kWeight1), std::logic_error);
}

TEST_F(GoalWeightsTest, EraseRemovesEntry) {
    store.set(&gl0, kWeight0);
    store.erase(&gl0);
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(GoalWeightsTest, EraseOnUnknownGoalThrows) {
    EXPECT_THROW(store.erase(&gl0), std::logic_error);
}

TEST_F(GoalWeightsTest, EraseDoesNotAffectOtherGoals) {
    store.set(&gl0, kWeight0);
    store.set(&gl1, kWeight1);
    store.erase(&gl0);
    EXPECT_DOUBLE_EQ(store.get(&gl1), kWeight1);
}

TEST_F(GoalWeightsTest, ClearGoalWeightsRemovesAllBindings) {
    store.set(&gl0, kWeight0);
    store.set(&gl1, kWeight1);
    store.clear_goal_weights();
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
    EXPECT_THROW(store.get(&gl1), std::out_of_range);
}
