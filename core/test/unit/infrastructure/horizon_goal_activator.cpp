// horizon_goal_activator: delegates to goal_activator, then assigns parent_w/g child weight.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/horizon_goal_activator.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/expr.hpp"

using ::testing::Return;

struct MockGoalActivator {
    MOCK_METHOD(void, activate, (const goal_lineage*));
};

struct MockGoalWeights {
    MOCK_METHOD(double, get, (const goal_lineage*), (const));
    MOCK_METHOD(void, set, (const goal_lineage*, double));
};

struct MockGetRule {
    MOCK_METHOD(const rule*, get, (rule_id), (const));
};

using TestHorizonGoalActivator = horizon_goal_activator<MockGoalActivator, MockGoalWeights, MockGetRule>;

struct HorizonGoalActivatorTest : public ::testing::Test {
    MockGoalActivator mock_goal_activator;
    MockGoalWeights goal_weights;
    MockGetRule get_rule;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 1};
    goal_lineage child_gl{&rl, 0};

    expr head{expr::var{0}};
    expr body0{expr::var{1}};
    expr body1{expr::var{2}};
    rule two_body_rule{&head, {&body0, &body1}, 3};

    static constexpr double kParentWeight = 1.0;
    static constexpr double kExpectedChildWeight = 0.5;

    TestHorizonGoalActivator activator{mock_goal_activator, goal_weights, get_rule};
};

TEST_F(HorizonGoalActivatorTest, DelegatesThenSetsParentWeightDividedByBodySize) {
    testing::InSequence seq;
    EXPECT_CALL(mock_goal_activator, activate(&child_gl)).Times(1);
    EXPECT_CALL(get_rule, get(rl.idx)).WillOnce(Return(&two_body_rule));
    EXPECT_CALL(goal_weights, get(&parent_gl)).WillOnce(Return(kParentWeight));
    EXPECT_CALL(goal_weights, set(&child_gl, kExpectedChildWeight)).Times(1);
    activator.activate(&child_gl);
}
