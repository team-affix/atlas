// horizon_tear_down_sim: snapshots horizon reward, tears down MCTS, clears weights, then state.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/horizon_tear_down_sim.hpp"

using ::testing::Return;

struct MockComputeMctsReward {
    MOCK_METHOD(double, compute_mcts_reward, (), (const));
};

struct MockSetValueDelta {
    MOCK_METHOD(void, set_value, (double));
};

struct MockTearDownMcts {
    MOCK_METHOD(void, tear_down, ());
};

struct MockGoalWeights {
    MOCK_METHOD(void, clear_goal_weights, ());
};

struct MockCumulativeGroundedWeight {
    MOCK_METHOD(void, clear, ());
};

struct MockTearDownSim {
    MOCK_METHOD(void, tear_down, ());
};

using test_horizon_tear_down_sim_t = horizon_tear_down_sim<
    MockComputeMctsReward, MockSetValueDelta, MockTearDownMcts,
    MockGoalWeights, MockCumulativeGroundedWeight, MockTearDownSim>;

struct HorizonTearDownSimTest : public ::testing::Test {
    MockComputeMctsReward compute_mcts_reward;
    MockSetValueDelta set_value_delta;
    MockTearDownMcts tear_down_mcts;
    MockGoalWeights goal_weights;
    MockCumulativeGroundedWeight cumulative_grounded_weight;
    MockTearDownSim mock_tear_down;
    test_horizon_tear_down_sim_t tear_down{
        compute_mcts_reward, set_value_delta, tear_down_mcts,
        goal_weights, cumulative_grounded_weight, mock_tear_down};
};

TEST_F(HorizonTearDownSimTest, SetsRewardTearsDownMctsClearsWeightsThenState) {
    testing::InSequence seq;
    EXPECT_CALL(compute_mcts_reward, compute_mcts_reward()).WillOnce(Return(0.5));
    EXPECT_CALL(set_value_delta, set_value(0.5)).Times(1);
    EXPECT_CALL(tear_down_mcts, tear_down()).Times(1);
    EXPECT_CALL(goal_weights, clear_goal_weights()).Times(1);
    EXPECT_CALL(cumulative_grounded_weight, clear()).Times(1);
    EXPECT_CALL(mock_tear_down, tear_down()).Times(1);
    tear_down.tear_down();
}
