// initial_goal_weight: immutable per-solver initial goal weight constant.

#include <gtest/gtest.h>
#include "infrastructure/initial_goal_weight.hpp"

TEST(InitialGoalWeightTest, GetReturnsConstructorValue) {
    static constexpr double kWeight = 0.3333333333333333;
    initial_goal_weight w{kWeight};
    EXPECT_DOUBLE_EQ(w.get(), kWeight);
}

TEST(InitialGoalWeightTest, ZeroWeightStored) {
    initial_goal_weight w{0.0};
    EXPECT_DOUBLE_EQ(w.get(), 0.0);
}
