// Resolver: delegates subgoal activation, candidate deactivation, and goal expr teardown.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/resolver.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_activate_subgoals.hpp"
#include "interfaces/i_deactivate_goal_candidates.hpp"

using ::testing::Return;

struct MockGoalDeactivator : public i_goal_deactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*), (override));
};

struct MockActivateSubgoals : public i_activate_subgoals {
    MOCK_METHOD(bool, activate_subgoals, (const resolution_lineage*), (override));
};

struct MockDeactivateGoalCandidates : public i_deactivate_goal_candidates {
    MOCK_METHOD(void, deactivate_goal_candidates, (const goal_lineage*), (override));
};

struct ResolverTest : public ::testing::Test {
    locator loc;
    MockGoalDeactivator goal_deactivator;
    MockActivateSubgoals activate_subgoals;
    MockDeactivateGoalCandidates deactivate_goal_candidates;
    resolver res;

    ResolverTest() : res(init_resolver()) {}

    resolver init_resolver() {
        loc.bind_as<i_goal_deactivator>(goal_deactivator);
        loc.bind_as<i_activate_subgoals>(activate_subgoals);
        loc.bind_as<i_deactivate_goal_candidates>(deactivate_goal_candidates);
        return resolver{loc};
    }

    static constexpr rule_id kRule = 0;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, kRule};
};

TEST_F(ResolverTest, ReturnsFalseWhenSubgoalsFail) {
    EXPECT_CALL(activate_subgoals, activate_subgoals(&rl)).WillOnce(Return(false));
    EXPECT_CALL(deactivate_goal_candidates, deactivate_goal_candidates).Times(0);
    EXPECT_CALL(goal_deactivator, deactivate).Times(0);
    EXPECT_FALSE(res.resolve(&rl));
}

TEST_F(ResolverTest, EmptyBodyDeactivatesParentOnly) {
    EXPECT_CALL(activate_subgoals, activate_subgoals(&rl)).WillOnce(Return(true));
    EXPECT_CALL(deactivate_goal_candidates, deactivate_goal_candidates(&parent_gl)).Times(1);
    EXPECT_CALL(goal_deactivator, deactivate(&parent_gl)).Times(1);
    EXPECT_TRUE(res.resolve(&rl));
}

TEST_F(ResolverTest, SuccessDeactivatesParentCandidatesAndGoal) {
    EXPECT_CALL(activate_subgoals, activate_subgoals(&rl)).WillOnce(Return(true));
    EXPECT_CALL(deactivate_goal_candidates, deactivate_goal_candidates(&parent_gl)).Times(1);
    EXPECT_CALL(goal_deactivator, deactivate(&parent_gl)).Times(1);
    EXPECT_TRUE(res.resolve(&rl));
}
