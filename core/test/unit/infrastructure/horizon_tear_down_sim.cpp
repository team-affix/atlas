// horizon_tear_down_sim: clears goal weights and CGW before base tear_down.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/horizon_tear_down_sim.hpp"

using ::testing::NiceMock;

struct MockTearDownSim {
    MOCK_METHOD(void, tear_down, ());
};

struct MockGoalWeights {
    MOCK_METHOD(void, clear_goal_weights, ());
};

struct MockCumulativeGroundedWeight {
    MOCK_METHOD(void, clear, ());
};

using TestHorizonTearDownSim = horizon_tear_down_sim<MockTearDownSim, MockGoalWeights, MockCumulativeGroundedWeight>;

struct HorizonTearDownSimTest : public ::testing::Test {
    MockGoalWeights goal_weights;
    MockCumulativeGroundedWeight cumulative_grounded_weight;
    MockTearDownSim mock_tear_down;
    TestHorizonTearDownSim tear_down{mock_tear_down, goal_weights, cumulative_grounded_weight};
};

TEST_F(HorizonTearDownSimTest, ClearsWeightsAndCgwBeforeBaseTearDown) {
    testing::InSequence seq;
    EXPECT_CALL(goal_weights, clear_goal_weights()).Times(1);
    EXPECT_CALL(cumulative_grounded_weight, clear()).Times(1);
    EXPECT_CALL(mock_tear_down, tear_down()).Times(1);
    tear_down.tear_down();
}
