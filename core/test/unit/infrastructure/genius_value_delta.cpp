// genius_value_delta: routes get_value_delta to horizon_reward (cgw) when the
// node is between decisions (second == nullptr), and to ridge_reward
// (-decision_count) when the node is goal-targeting (second != nullptr).

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/genius_value_delta.hpp"
#include "value_objects/mcts_node_id.hpp"

using ::testing::Return;

namespace {

struct MockRidgeReward {
    MOCK_METHOD(double, compute_mcts_reward, (), (const));
};

struct MockHorizonReward {
    MOCK_METHOD(double, compute_mcts_reward, (), (const));
};

} // namespace

using test_genius_value_delta_t = genius_value_delta<MockRidgeReward, MockHorizonReward>;

struct GeniusValueDeltaTest : public ::testing::Test {
    MockRidgeReward   ridge;
    MockHorizonReward horizon;
    test_genius_value_delta_t delta{ridge, horizon};

    goal_lineage gl{nullptr, 0};
};

// ── between-decisions node (second == nullptr) ────────────────────────────────

TEST_F(GeniusValueDeltaTest, BetweenDecisionsUsesHorizonReward) {
    mcts_node_id node{{}, nullptr};
    EXPECT_CALL(ridge,   compute_mcts_reward()).Times(0);
    EXPECT_CALL(horizon, compute_mcts_reward()).WillOnce(Return(0.75));
    EXPECT_DOUBLE_EQ(delta.get_value_delta(node), 0.75);
}

TEST_F(GeniusValueDeltaTest, BetweenDecisionsDoesNotCallRidgeReward) {
    mcts_node_id node{{}, nullptr};
    EXPECT_CALL(ridge, compute_mcts_reward()).Times(0);
    EXPECT_CALL(horizon, compute_mcts_reward()).WillOnce(Return(0.0));
    delta.get_value_delta(node);
}

TEST_F(GeniusValueDeltaTest, BetweenDecisionsReturnsHorizonValue) {
    mcts_node_id node{{}, nullptr};
    EXPECT_CALL(ridge,   compute_mcts_reward()).Times(0);
    EXPECT_CALL(horizon, compute_mcts_reward()).WillOnce(Return(-3.5));
    EXPECT_DOUBLE_EQ(delta.get_value_delta(node), -3.5);
}

// ── goal-targeting node (second != nullptr) ───────────────────────────────────

TEST_F(GeniusValueDeltaTest, GoalTargetingUsesRidgeReward) {
    mcts_node_id node{{}, &gl};
    EXPECT_CALL(horizon, compute_mcts_reward()).Times(0);
    EXPECT_CALL(ridge,   compute_mcts_reward()).WillOnce(Return(-2.0));
    EXPECT_DOUBLE_EQ(delta.get_value_delta(node), -2.0);
}

TEST_F(GeniusValueDeltaTest, GoalTargetingDoesNotCallHorizonReward) {
    mcts_node_id node{{}, &gl};
    EXPECT_CALL(horizon, compute_mcts_reward()).Times(0);
    EXPECT_CALL(ridge, compute_mcts_reward()).WillOnce(Return(0.0));
    delta.get_value_delta(node);
}

TEST_F(GeniusValueDeltaTest, GoalTargetingReturnsRidgeValue) {
    mcts_node_id node{{}, &gl};
    EXPECT_CALL(horizon, compute_mcts_reward()).Times(0);
    EXPECT_CALL(ridge,   compute_mcts_reward()).WillOnce(Return(-7.0));
    EXPECT_DOUBLE_EQ(delta.get_value_delta(node), -7.0);
}
