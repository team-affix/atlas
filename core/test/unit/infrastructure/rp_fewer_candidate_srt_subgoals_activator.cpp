// rp_fewer_candidate_srt_subgoals_activator: activate, link, set batch values, flush.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/rp_fewer_candidate_srt_subgoals_activator.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "value_objects/lineage.hpp"

using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockSubgoalsActivator {
    MOCK_METHOD(bool, activate_subgoals_and_candidates, (const resolution_lineage*));
};

struct MockLinkSrtGoalBatchParent {
    MOCK_METHOD(void, link_srt_goal_batch_parent, (const goal_lineage*));
};

struct MockComputeActiveGoalValue {
    MOCK_METHOD(double, compute_active_goal_value, (const goal_lineage*));
};

struct MockSetActiveGoalValue {
    MOCK_METHOD(void, set_active_goal_value, (const goal_lineage*, double));
};

using activator_t = rp_fewer_candidate_srt_subgoals_activator<
    MockSubgoalsActivator, MockLinkSrtGoalBatchParent, srt_active_goals,
    srt_active_goals, MockComputeActiveGoalValue, MockSetActiveGoalValue>;

}  // namespace

struct RpFewerCandidateSrtSubgoalsActivatorTest : public ::testing::Test {
    StrictMock<MockSubgoalsActivator> subgoals;
    StrictMock<MockLinkSrtGoalBatchParent> link;
    srt_active_goals srt;
    StrictMock<MockComputeActiveGoalValue> compute;
    StrictMock<MockSetActiveGoalValue> set_value;
    activator_t activator{subgoals, link, srt, srt, compute, set_value};

    goal_lineage parent{nullptr, 0};
    goal_lineage child0{nullptr, 1};
    goal_lineage child1{nullptr, 2};
    resolution_lineage rl{&parent, 7};
};

TEST_F(RpFewerCandidateSrtSubgoalsActivatorTest, SetsBatchAfterLinkBeforeFlush) {
    srt.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();
    srt.insert_active_goal(&child0);
    srt.insert_active_goal(&child1);

    EXPECT_CALL(subgoals, activate_subgoals_and_candidates(&rl)).WillOnce(Return(true));
    EXPECT_CALL(link, link_srt_goal_batch_parent(&parent));
    EXPECT_CALL(compute, compute_active_goal_value(&child0)).WillOnce(Return(-2.0));
    EXPECT_CALL(compute, compute_active_goal_value(&child1)).WillOnce(Return(-5.0));
    EXPECT_CALL(set_value, set_active_goal_value(&child0, -2.0));
    EXPECT_CALL(set_value, set_active_goal_value(&child1, -5.0));
    EXPECT_TRUE(activator.activate_subgoals_and_candidates(&rl));
}

TEST_F(RpFewerCandidateSrtSubgoalsActivatorTest, EmptyBatchSkipsSet) {
    srt.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();

    InSequence seq;
    EXPECT_CALL(subgoals, activate_subgoals_and_candidates(&rl)).WillOnce(Return(true));
    EXPECT_CALL(link, link_srt_goal_batch_parent(&parent));
    EXPECT_CALL(set_value, set_active_goal_value).Times(0);
    EXPECT_TRUE(activator.activate_subgoals_and_candidates(&rl));
}

TEST_F(RpFewerCandidateSrtSubgoalsActivatorTest, PropagatesInnerFalseWithoutLink) {
    EXPECT_CALL(subgoals, activate_subgoals_and_candidates(&rl)).WillOnce(Return(false));
    EXPECT_CALL(link, link_srt_goal_batch_parent).Times(0);
    EXPECT_CALL(set_value, set_active_goal_value).Times(0);
    EXPECT_FALSE(activator.activate_subgoals_and_candidates(&rl));
}
