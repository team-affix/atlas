// goal_work_function: f(l) = 1 + exp(-K(l-J)), where l is the depth the injected
// getter reports for the goal. With K=0.2, J=10: f(J)=2, monotone in l, asymptote
// → 1 from above.

#include <cmath>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_work_function.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetGoalDepth {
    MOCK_METHOD(size_t, get, (const goal_lineage*), (const));
};

using test_goal_work_function_t = goal_work_function<NiceMock<MockGetGoalDepth>>;

}  // namespace

struct GoalWorkFunctionTest : public ::testing::Test {
    static constexpr double kK = 0.2;
    static constexpr double kJ = 10.0;

    NiceMock<MockGetGoalDepth> get_goal_depth;
    test_goal_work_function_t work_fn{get_goal_depth, kK, kJ};

    goal_lineage at0{nullptr, 0};
    goal_lineage at1{nullptr, 1};
    goal_lineage at10{nullptr, 2};
    goal_lineage at100{nullptr, 3};
    goal_lineage at1000{nullptr, 4};

    void SetUp() override {
        ON_CALL(get_goal_depth, get(&at0)).WillByDefault(Return(0u));
        ON_CALL(get_goal_depth, get(&at1)).WillByDefault(Return(1u));
        ON_CALL(get_goal_depth, get(&at10)).WillByDefault(Return(10u));
        ON_CALL(get_goal_depth, get(&at100)).WillByDefault(Return(100u));
        ON_CALL(get_goal_depth, get(&at1000)).WillByDefault(Return(1000u));
    }
};

TEST_F(GoalWorkFunctionTest, ResolvesDepthThroughInjectedGetter) {
    // The caller hands over a goal and nothing else; the depth is the work
    // function's own business, so a future work function can ignore it entirely.
    EXPECT_CALL(get_goal_depth, get(&at10)).WillOnce(Return(10u));
    EXPECT_DOUBLE_EQ(work_fn.get(&at10), 2.0);
}

TEST_F(GoalWorkFunctionTest, AtJEqualsTwo) {
    EXPECT_DOUBLE_EQ(work_fn.get(&at10), 2.0);
}

TEST_F(GoalWorkFunctionTest, DecreasesWithDepth) {
    EXPECT_GT(work_fn.get(&at0), work_fn.get(&at10));
    EXPECT_GT(work_fn.get(&at10), work_fn.get(&at100));
}

TEST_F(GoalWorkFunctionTest, ApproachesOneFromAboveForLargeDepth) {
    const double f_large = work_fn.get(&at100);
    EXPECT_GT(f_large, 1.0);
    EXPECT_NEAR(f_large, 1.0, 1e-6);
}

TEST_F(GoalWorkFunctionTest, ValueAtDepthZeroMatchesClosedForm) {
    // Every initial goal is credited f(0), so this constant sets the scale of
    // remaining_work and therefore of the quell reward. It has only ever been used
    // via f0() in the conservation tests, never pinned to its closed form.
    EXPECT_DOUBLE_EQ(work_fn.get(&at0), 1.0 + std::exp(kK * kJ));
}

TEST_F(GoalWorkFunctionTest, ZeroDecayGivesConstantTwoAtEveryDepth) {
    // K = 0 removes the depth term entirely, so every goal costs the same. This is
    // the degenerate configuration that makes remaining_work a plain goal count;
    // a sign slip on K would show up here as a non-constant result.
    test_goal_work_function_t flat{get_goal_depth, 0.0, kJ};
    EXPECT_DOUBLE_EQ(flat.get(&at0), 2.0);
    EXPECT_DOUBLE_EQ(flat.get(&at1), 2.0);
    EXPECT_DOUBLE_EQ(flat.get(&at1000), 2.0);
}
