// horizon_initial_goal_activator: delegates to initial_goal_activator, then assigns
// cached initial goal weight from get_initial_goal_weight.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/horizon_initial_goal_activator.hpp"
#include "value_objects/expr.hpp"

using ::testing::Return;

struct MockInitialGoalActivator {
    MOCK_METHOD(void, activate_initial_goal, (subgoal_id));
};

struct MockMakeInitialGoalLineage {
    MOCK_METHOD((const goal_lineage*), make, (subgoal_id));
};

struct MockGoalWeights {
    MOCK_METHOD(void, set, (const goal_lineage*, double));
};

struct MockInitialGoalWeight {
    MOCK_METHOD(double, get, (), (const));
};

using TestHorizonInitialGoalActivator = horizon_initial_goal_activator<
    MockInitialGoalActivator, MockMakeInitialGoalLineage,
    MockGoalWeights, MockInitialGoalWeight>;

struct HorizonInitialGoalActivatorTest : public ::testing::Test {
    MockInitialGoalActivator mock_initial;
    MockMakeInitialGoalLineage make_initial_goal_lineage;
    MockGoalWeights goal_weights;
    MockInitialGoalWeight initial_goal_weight;

    goal_lineage gl{nullptr, 0};
    static constexpr subgoal_id kIdx = 0;
    static constexpr double kInitialWeight = 0.5;

    TestHorizonInitialGoalActivator activator{mock_initial, make_initial_goal_lineage,
                                              goal_weights, initial_goal_weight};
};

TEST_F(HorizonInitialGoalActivatorTest, DelegatesThenSetsCachedInitialWeight) {
    testing::InSequence seq;
    EXPECT_CALL(mock_initial, activate_initial_goal(kIdx)).Times(1);
    EXPECT_CALL(make_initial_goal_lineage, make(kIdx)).WillOnce(Return(&gl));
    EXPECT_CALL(initial_goal_weight, get()).WillOnce(Return(kInitialWeight));
    EXPECT_CALL(goal_weights, set(&gl, kInitialWeight)).Times(1);
    activator.activate_initial_goal(kIdx);
}
