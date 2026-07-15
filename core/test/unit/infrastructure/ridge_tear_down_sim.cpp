// ridge_tear_down_sim: snapshots ridge reward, tears down MCTS, then state.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/ridge_tear_down_sim.hpp"

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

struct MockTearDownSim {
    MOCK_METHOD(void, tear_down, ());
};

using test_ridge_tear_down_sim_t = ridge_tear_down_sim<
    MockComputeMctsReward, MockSetValueDelta, MockTearDownMcts, MockTearDownSim>;

struct RidgeTearDownSimTest : public ::testing::Test {
    MockComputeMctsReward compute_mcts_reward;
    MockSetValueDelta set_value_delta;
    MockTearDownMcts tear_down_mcts;
    MockTearDownSim tear_down_sim;
    test_ridge_tear_down_sim_t tear_down{
        compute_mcts_reward, set_value_delta, tear_down_mcts, tear_down_sim};
};

TEST_F(RidgeTearDownSimTest, SetsRewardThenTearsDownMctsThenState) {
    testing::InSequence seq;
    EXPECT_CALL(compute_mcts_reward, compute_mcts_reward()).WillOnce(Return(-3.0));
    EXPECT_CALL(set_value_delta, set_value(-3.0)).Times(1);
    EXPECT_CALL(tear_down_mcts, tear_down()).Times(1);
    EXPECT_CALL(tear_down_sim, tear_down()).Times(1);
    tear_down.tear_down();
}
