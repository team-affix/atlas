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

TEST_F(GoalWorkFunctionTest, ValueAtDepthZeroMatchesClosedForm) {
    // Every initial goal is credited f(0), so this constant sets the scale of
    // remaining_work and therefore of the quell reward. It has only ever been used
    // via f0() in the conservation tests, never pinned to its closed form.
    EXPECT_DOUBLE_EQ(work_fn.get(0), 1.0 + std::exp(kK * kJ));
}

TEST_F(GoalWorkFunctionTest, ZeroDecayGivesConstantTwoAtEveryDepth) {
    // K = 0 removes the depth term entirely, so every goal costs the same. This is
    // the degenerate configuration that makes remaining_work a plain goal count;
    // a sign slip on K would show up here as a non-constant result.
    goal_work_function flat{0.0, kJ};
    EXPECT_DOUBLE_EQ(flat.get(0), 2.0);
    EXPECT_DOUBLE_EQ(flat.get(1), 2.0);
    EXPECT_DOUBLE_EQ(flat.get(1000), 2.0);
}
