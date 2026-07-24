// goal_work_function: f(l) = 1 + exp(-K(l-J)). With K=0.2, J=10: f(J)=2, monotone in l,
// asymptote → 1 from above.

#include <cmath>
#include <gtest/gtest.h>
#include "infrastructure/goal_work_function.hpp"

struct GoalWorkFunctionTest : public ::testing::Test {
    static constexpr double kK = 0.2;
    static constexpr double kJ = 10.0;
    goal_work_function work_fn{kK, kJ};
};

TEST_F(GoalWorkFunctionTest, AtJEqualsTwo) {
    EXPECT_DOUBLE_EQ(work_fn.get(static_cast<size_t>(kJ)), 2.0);
}

TEST_F(GoalWorkFunctionTest, DecreasesWithDepth) {
    EXPECT_GT(work_fn.get(0), work_fn.get(10));
    EXPECT_GT(work_fn.get(10), work_fn.get(100));
}

TEST_F(GoalWorkFunctionTest, ApproachesOneFromAboveForLargeDepth) {
    const double f_large = work_fn.get(100);
    EXPECT_GT(f_large, 1.0);
    EXPECT_NEAR(f_large, 1.0, 1e-6);
}
