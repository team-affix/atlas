// srt_initial_goals_activator: delegates to initial_goals_activator, then flushes SRT goal batch.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "locator_fixture.hpp"
#include "infrastructure/srt_initial_goals_activator.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "interfaces/i_get_initial_goal_count.hpp"
#include "interfaces/i_activate_initial_goal.hpp"
#include "interfaces/i_make_initial_goal_lineage.hpp"
#include "interfaces/i_activate_goal_candidates.hpp"
#include "interfaces/i_srt_flush_goal_batch.hpp"

using ::testing::Return;

struct MockGetInitialGoalCount : public i_get_initial_goal_count {
    MOCK_METHOD(size_t, count, (), (const, override));
};

struct MockActivateInitialGoal : public i_activate_initial_goal {
    MOCK_METHOD(void, activate_initial_goal, (subgoal_id), (override));
};

struct MockMakeInitialGoalLineage : public i_make_initial_goal_lineage {
    MOCK_METHOD(const goal_lineage*, make, (subgoal_id), (override));
};

struct MockActivateGoalCandidates : public i_activate_goal_candidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*), (override));
};

struct MockInitialGoalsActivator : public initial_goals_activator {
    explicit MockInitialGoalsActivator(locator& loc) : initial_goals_activator(loc) {}
    MOCK_METHOD(bool, activate_initial_goals_and_candidates, (), (override));
};

struct MockSrtFlushGoalBatch : public i_srt_flush_goal_batch {
    MOCK_METHOD(void, flush_srt_goal_batch, (), (override));
};

struct SrtInitialGoalsActivatorTest : public ::testing::Test {
    locator loc;
    MockGetInitialGoalCount get_initial_goal_count;
    MockActivateInitialGoal activate_initial_goal;
    MockMakeInitialGoalLineage make_initial_goal_lineage;
    MockActivateGoalCandidates activate_goal_candidates;
    MockSrtFlushGoalBatch flush_batch;
    std::unique_ptr<MockInitialGoalsActivator> mock_initial;
    std::unique_ptr<srt_initial_goals_activator> activator;

    void SetUp() override {
        loc.bind_as<i_get_initial_goal_count>(get_initial_goal_count);
        loc.bind_as<i_activate_initial_goal>(activate_initial_goal);
        loc.bind_as<i_make_initial_goal_lineage>(make_initial_goal_lineage);
        loc.bind_as<i_activate_goal_candidates>(activate_goal_candidates);
        mock_initial = std::make_unique<MockInitialGoalsActivator>(loc);
        loc.bind_as<initial_goals_activator>(*mock_initial);
        loc.bind_as<i_srt_flush_goal_batch>(flush_batch);
        activator = std::make_unique<srt_initial_goals_activator>(loc);
    }
};

TEST_F(SrtInitialGoalsActivatorTest, DelegatesThenFlushesInOrder) {
    testing::InSequence seq;
    EXPECT_CALL(*mock_initial, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(flush_batch, flush_srt_goal_batch()).Times(1);
    EXPECT_TRUE(activator->activate_initial_goals_and_candidates());
}

TEST_F(SrtInitialGoalsActivatorTest, PropagatesInnerFalseWithoutFlush) {
    EXPECT_CALL(*mock_initial, activate_initial_goals_and_candidates()).WillOnce(Return(false));
    EXPECT_CALL(flush_batch, flush_srt_goal_batch()).Times(0);
    EXPECT_FALSE(activator->activate_initial_goals_and_candidates());
}
