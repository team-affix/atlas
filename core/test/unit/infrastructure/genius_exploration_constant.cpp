// genius_exploration_constant: horizon c when parent pending-goal is an active
// leaf (children are rule choices); ridge c otherwise.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/genius_exploration_constant.hpp"
#include "value_objects/mcts_scope_node_id.hpp"

using ::testing::Return;

namespace {

struct MockIsActiveGoal {
    MOCK_METHOD(bool, is_active_goal, (const goal_lineage*), (const));
};

} // namespace

using test_genius_exploration_constant_t = genius_exploration_constant<MockIsActiveGoal>;

struct GeniusExplorationConstantTest : public ::testing::Test {
    static constexpr double kRidge = 7.0;
    static constexpr double kHorizon = 0.5;

    MockIsActiveGoal is_active_goal;
    test_genius_exploration_constant_t sut{is_active_goal, kRidge, kHorizon};

    goal_lineage gl{nullptr, 0};
};

TEST_F(GeniusExplorationConstantTest, ActiveGoalParentUsesHorizonConstant) {
    mcts_scope_node_id node{{}, &gl};
    EXPECT_CALL(is_active_goal, is_active_goal(&gl)).WillOnce(Return(true));
    EXPECT_DOUBLE_EQ(sut.get_exploration_constant(node), kHorizon);
}

TEST_F(GeniusExplorationConstantTest, InactiveGoalParentUsesRidgeConstant) {
    mcts_scope_node_id node{{}, &gl};
    EXPECT_CALL(is_active_goal, is_active_goal(&gl)).WillOnce(Return(false));
    EXPECT_DOUBLE_EQ(sut.get_exploration_constant(node), kRidge);
}

TEST_F(GeniusExplorationConstantTest, NullPendingGoalUsesRidgeConstant) {
    mcts_scope_node_id node{{}, nullptr};
    EXPECT_CALL(is_active_goal, is_active_goal(nullptr)).WillOnce(Return(false));
    EXPECT_DOUBLE_EQ(sut.get_exploration_constant(node), kRidge);
}
