// genius_tear_down_sim: tears down MCTS while weights live, then clears and teardowns state.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/genius_tear_down_sim.hpp"

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

using test_genius_tear_down_sim_t = genius_tear_down_sim<
    MockTearDownMcts, MockGoalWeights, MockCumulativeGroundedWeight, MockTearDownSim>;

struct GeniusTearDownSimTest : public ::testing::Test {
    MockTearDownMcts tear_down_mcts;
    MockGoalWeights goal_weights;
    MockCumulativeGroundedWeight cumulative_grounded_weight;
    MockTearDownSim mock_tear_down;
    test_genius_tear_down_sim_t tear_down{
        tear_down_mcts, goal_weights, cumulative_grounded_weight, mock_tear_down};
};

TEST_F(GeniusTearDownSimTest, TearsDownMctsBeforeClearingWeightsAndState) {
    testing::InSequence seq;
    EXPECT_CALL(tear_down_mcts, tear_down()).Times(1);
    EXPECT_CALL(goal_weights, clear_goal_weights()).Times(1);
    EXPECT_CALL(cumulative_grounded_weight, clear()).Times(1);
    EXPECT_CALL(mock_tear_down, tear_down()).Times(1);
    tear_down.tear_down();
}
