// quell_tear_down_sim: snapshots quell reward, tears down MCTS, clears depths/work/
// remaining, then state.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/quell_tear_down_sim.hpp"

using ::testing::AtLeast;
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

struct MockClearGoalDepths {
    MOCK_METHOD(void, clear_goal_depths, ());
};

struct MockClearGoalWorkValues {
    MOCK_METHOD(void, clear_goal_work_values, ());
};

struct MockClearRemainingWork {
    MOCK_METHOD(void, clear, ());
};

struct MockTearDownSim {
    MOCK_METHOD(void, tear_down, ());
};

using test_quell_tear_down_sim_t = quell_tear_down_sim<
    MockComputeMctsReward, MockSetValueDelta, MockTearDownMcts,
    MockClearGoalDepths, MockClearGoalWorkValues, MockClearRemainingWork, MockTearDownSim>;

struct QuellTearDownSimTest : public ::testing::Test {
    MockComputeMctsReward compute_mcts_reward;
    MockSetValueDelta set_value_delta;
    MockTearDownMcts tear_down_mcts;
    MockClearGoalDepths clear_goal_depths;
    MockClearGoalWorkValues clear_goal_work_values;
    MockClearRemainingWork clear_remaining_work;
    MockTearDownSim mock_tear_down;
    test_quell_tear_down_sim_t tear_down{
        compute_mcts_reward, set_value_delta, tear_down_mcts,
        clear_goal_depths, clear_goal_work_values, clear_remaining_work, mock_tear_down};
};

TEST_F(QuellTearDownSimTest, SetsRewardTearsDownMctsClearsThenState) {
    EXPECT_CALL(compute_mcts_reward, compute_mcts_reward())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(0.5));
    testing::InSequence seq;
    EXPECT_CALL(set_value_delta, set_value(0.5)).Times(1);
    EXPECT_CALL(tear_down_mcts, tear_down()).Times(1);
    EXPECT_CALL(clear_goal_depths, clear_goal_depths()).Times(1);
    EXPECT_CALL(clear_goal_work_values, clear_goal_work_values()).Times(1);
    EXPECT_CALL(clear_remaining_work, clear()).Times(1);
    EXPECT_CALL(mock_tear_down, tear_down()).Times(1);
    tear_down.tear_down();
}
