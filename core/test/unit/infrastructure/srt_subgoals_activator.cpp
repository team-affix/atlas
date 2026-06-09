// srt_subgoals_activator: delegates to subgoals_activator, then links batch parent and flushes.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "locator_fixture.hpp"
#include "infrastructure/srt_subgoals_activator.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "interfaces/i_make_goal_lineage.hpp"
#include "interfaces/i_goal_activator.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_activate_goal_candidates.hpp"
#include "interfaces/i_srt_link_goal_batch_parent.hpp"
#include "interfaces/i_srt_flush_goal_batch.hpp"

using ::testing::Return;

struct MockMakeGoalLineage : public i_make_goal_lineage {
    MOCK_METHOD((const goal_lineage*), make_goal_lineage,
        (const resolution_lineage*, subgoal_id), (override));
};

struct MockGoalActivator : public i_goal_activator {
    MOCK_METHOD(void, activate, (const goal_lineage*), (override));
};

struct MockGetRule : public i_get_rule {
    MOCK_METHOD(const rule*, get, (rule_id), (const, override));
};

struct MockActivateGoalCandidates : public i_activate_goal_candidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*), (override));
};

struct MockSubgoalsActivator : public subgoals_activator {
    explicit MockSubgoalsActivator(locator& loc) : subgoals_activator(loc) {}
    MOCK_METHOD(bool, activate_subgoals_and_candidates, (const resolution_lineage*), (override));
};

struct MockSrtLinkGoalBatchParent : public i_srt_link_goal_batch_parent {
    MOCK_METHOD(void, link_srt_goal_batch_parent, (const goal_lineage*), (override));
};

struct MockSrtFlushGoalBatch : public i_srt_flush_goal_batch {
    MOCK_METHOD(void, flush_srt_goal_batch, (), (override));
};

struct SrtSubgoalsActivatorTest : public ::testing::Test {
    locator loc;
    MockMakeGoalLineage make_goal_lineage;
    MockGoalActivator goal_activator;
    MockGetRule get_rule;
    MockActivateGoalCandidates activate_goal_candidates;
    MockSrtLinkGoalBatchParent link_batch;
    MockSrtFlushGoalBatch flush_batch;
    std::unique_ptr<MockSubgoalsActivator> mock_subgoals;
    std::unique_ptr<srt_subgoals_activator> activator;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 1};

    void SetUp() override {
        loc.bind_as<i_make_goal_lineage>(make_goal_lineage);
        loc.bind_as<i_goal_activator>(goal_activator);
        loc.bind_as<i_get_rule>(get_rule);
        loc.bind_as<i_activate_goal_candidates>(activate_goal_candidates);
        mock_subgoals = std::make_unique<MockSubgoalsActivator>(loc);
        loc.bind_as<subgoals_activator>(*mock_subgoals);
        loc.bind_as<i_srt_link_goal_batch_parent>(link_batch);
        loc.bind_as<i_srt_flush_goal_batch>(flush_batch);
        activator = std::make_unique<srt_subgoals_activator>(loc);
    }
};

TEST_F(SrtSubgoalsActivatorTest, DelegatesThenLinksParentAndFlushesInOrder) {
    testing::InSequence seq;
    EXPECT_CALL(*mock_subgoals, activate_subgoals_and_candidates(&rl)).WillOnce(Return(true));
    EXPECT_CALL(link_batch, link_srt_goal_batch_parent(&parent_gl)).Times(1);
    EXPECT_CALL(flush_batch, flush_srt_goal_batch()).Times(1);
    EXPECT_TRUE(activator->activate_subgoals_and_candidates(&rl));
}

TEST_F(SrtSubgoalsActivatorTest, PropagatesInnerFalseWithoutLinkOrFlush) {
    EXPECT_CALL(*mock_subgoals, activate_subgoals_and_candidates(&rl)).WillOnce(Return(false));
    EXPECT_CALL(link_batch, link_srt_goal_batch_parent).Times(0);
    EXPECT_CALL(flush_batch, flush_srt_goal_batch()).Times(0);
    EXPECT_FALSE(activator->activate_subgoals_and_candidates(&rl));
}
