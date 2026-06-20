// srt_subgoals_activator: delegates to subgoals_activator, then links batch parent and flushes.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/srt_subgoals_activator.hpp"

using ::testing::Return;

struct MockSubgoalsActivator {
    MOCK_METHOD(bool, activate_subgoals_and_candidates, (const resolution_lineage*));
};

struct MockSrtActiveGoals {
    MOCK_METHOD(void, link_srt_goal_batch_parent, (const goal_lineage*));
    MOCK_METHOD(void, flush_srt_goal_batch, ());
};

using TestSrtSubgoalsActivator = srt_subgoals_activator<MockSrtActiveGoals, MockSubgoalsActivator>;

struct SrtSubgoalsActivatorTest : public ::testing::Test {
    MockSrtActiveGoals srt_active_goals;
    MockSubgoalsActivator subgoals_activator;
    TestSrtSubgoalsActivator activator{srt_active_goals, subgoals_activator};

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 1};
};

TEST_F(SrtSubgoalsActivatorTest, DelegatesThenLinksParentAndFlushesInOrder) {
    testing::InSequence seq;
    EXPECT_CALL(subgoals_activator, activate_subgoals_and_candidates(&rl)).WillOnce(Return(true));
    EXPECT_CALL(srt_active_goals, link_srt_goal_batch_parent(&parent_gl)).Times(1);
    EXPECT_CALL(srt_active_goals, flush_srt_goal_batch()).Times(1);
    EXPECT_TRUE(activator.activate_subgoals_and_candidates(&rl));
}

TEST_F(SrtSubgoalsActivatorTest, PropagatesInnerFalseWithoutLinkOrFlush) {
    EXPECT_CALL(subgoals_activator, activate_subgoals_and_candidates(&rl)).WillOnce(Return(false));
    EXPECT_CALL(srt_active_goals, link_srt_goal_batch_parent).Times(0);
    EXPECT_CALL(srt_active_goals, flush_srt_goal_batch()).Times(0);
    EXPECT_FALSE(activator.activate_subgoals_and_candidates(&rl));
}
