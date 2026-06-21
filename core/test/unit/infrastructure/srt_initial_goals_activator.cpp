// srt_initial_goals_activator: delegates to initial_goals_activator, then flushes SRT goal batch.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/srt_initial_goals_activator.hpp"

using ::testing::Return;

struct MockInitialGoalsActivator {
    MOCK_METHOD(bool, activate_initial_goals_and_candidates, ());
};

struct MockSrtActiveGoals {
    MOCK_METHOD(void, flush_srt_goal_batch, ());
};

using test_srt_initial_goals_activator_t = srt_initial_goals_activator<MockSrtActiveGoals, MockInitialGoalsActivator>;

struct SrtInitialGoalsActivatorTest : public ::testing::Test {
    MockSrtActiveGoals srt_active_goals;
    MockInitialGoalsActivator initial_goals_activator;
    test_srt_initial_goals_activator_t activator{srt_active_goals, initial_goals_activator};
};

TEST_F(SrtInitialGoalsActivatorTest, DelegatesThenFlushesInOrder) {
    testing::InSequence seq;
    EXPECT_CALL(initial_goals_activator, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(srt_active_goals, flush_srt_goal_batch()).Times(1);
    EXPECT_TRUE(activator.activate_initial_goals_and_candidates());
}

TEST_F(SrtInitialGoalsActivatorTest, PropagatesInnerFalseWithoutFlush) {
    EXPECT_CALL(initial_goals_activator, activate_initial_goals_and_candidates()).WillOnce(Return(false));
    EXPECT_CALL(srt_active_goals, flush_srt_goal_batch()).Times(0);
    EXPECT_FALSE(activator.activate_initial_goals_and_candidates());
}
