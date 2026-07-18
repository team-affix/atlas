// rp_fewer_candidate_srt_subgoals_activator: activate, link, set batch values, flush.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/rp_fewer_candidate_srt_subgoals_activator.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "value_objects/lineage.hpp"

using ::testing::ByMove;
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

struct MockIterateSrtGoalBatch {
    MOCK_METHOD((coroutine<const goal_lineage*, void>), iterate_srt_goal_batch, (),
                (const));
};

struct MockFlushSrtGoalBatch {
    MOCK_METHOD(void, flush_srt_goal_batch, ());
};

struct MockComputeActiveGoalValue {
    MOCK_METHOD(double, compute_active_goal_value, (const goal_lineage*));
};

struct MockSetActiveGoalValue {
    MOCK_METHOD(void, set_active_goal_value, (const goal_lineage*, double));
};

coroutine<const goal_lineage*, void> two_batch_sm(const goal_lineage* a,
                                                 const goal_lineage* b) {
    co_yield a;
    co_yield b;
}

using activator_t = rp_fewer_candidate_srt_subgoals_activator<
    MockSubgoalsActivator, MockLinkSrtGoalBatchParent, srt_active_goals,
    srt_active_goals, MockComputeActiveGoalValue, MockSetActiveGoalValue>;

using activator_flush_mocked_t = rp_fewer_candidate_srt_subgoals_activator<
    MockSubgoalsActivator, MockLinkSrtGoalBatchParent, MockIterateSrtGoalBatch,
    MockFlushSrtGoalBatch, MockComputeActiveGoalValue, MockSetActiveGoalValue>;

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

struct RpFewerCandidateSrtSubgoalsFlushOrderTest : public ::testing::Test {
    StrictMock<MockSubgoalsActivator> subgoals;
    StrictMock<MockLinkSrtGoalBatchParent> link;
    StrictMock<MockIterateSrtGoalBatch> iterate_batch;
    StrictMock<MockFlushSrtGoalBatch> flush;
    StrictMock<MockComputeActiveGoalValue> compute;
    StrictMock<MockSetActiveGoalValue> set_value;
    activator_flush_mocked_t activator{
        subgoals, link, iterate_batch, flush, compute, set_value};

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

TEST_F(RpFewerCandidateSrtSubgoalsFlushOrderTest, FlushAfterAllBatchSets) {
    InSequence seq;
    EXPECT_CALL(subgoals, activate_subgoals_and_candidates(&rl)).WillOnce(Return(true));
    EXPECT_CALL(link, link_srt_goal_batch_parent(&parent));
    EXPECT_CALL(iterate_batch, iterate_srt_goal_batch())
        .WillOnce(Return(ByMove(two_batch_sm(&child0, &child1))));
    EXPECT_CALL(compute, compute_active_goal_value(&child0)).WillOnce(Return(-2.0));
    EXPECT_CALL(set_value, set_active_goal_value(&child0, -2.0));
    EXPECT_CALL(compute, compute_active_goal_value(&child1)).WillOnce(Return(-5.0));
    EXPECT_CALL(set_value, set_active_goal_value(&child1, -5.0));
    EXPECT_CALL(flush, flush_srt_goal_batch());
    EXPECT_TRUE(activator.activate_subgoals_and_candidates(&rl));
}
