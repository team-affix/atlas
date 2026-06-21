// resolver_t: delegates subgoal activation, candidate deactivation, and goal expr teardown.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/resolver.hpp"

using ::testing::Return;

struct MockGoalDeactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*));
};

struct MockActivateSubgoalsAndCandidates {
    MOCK_METHOD(bool, activate_subgoals_and_candidates, (const resolution_lineage*));
};

struct MockDeactivateGoalCandidates {
    MOCK_METHOD(void, deactivate_goal_candidates, (const goal_lineage*));
};

struct MockSetChosenGoalCandidate {
    MOCK_METHOD(void, set, (const goal_lineage*, rule_id));
};

using test_resolver_t = resolver<MockGoalDeactivator, MockActivateSubgoalsAndCandidates,
                               MockDeactivateGoalCandidates, MockSetChosenGoalCandidate>;

struct ResolverTest : public ::testing::Test {
    MockGoalDeactivator goal_deactivator;
    MockActivateSubgoalsAndCandidates activate_subgoals_and_candidates;
    MockDeactivateGoalCandidates deactivate_goal_candidates;
    MockSetChosenGoalCandidate set_chosen_goal_candidate;
    test_resolver_t res{goal_deactivator, activate_subgoals_and_candidates,
                     deactivate_goal_candidates, set_chosen_goal_candidate};

    static constexpr rule_id kRule = 0;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, kRule};
};

TEST_F(ResolverTest, ReturnsFalseWhenSubgoalsFail) {
    EXPECT_CALL(activate_subgoals_and_candidates, activate_subgoals_and_candidates(&rl)).WillOnce(Return(false));
    EXPECT_CALL(deactivate_goal_candidates, deactivate_goal_candidates).Times(0);
    EXPECT_CALL(goal_deactivator, deactivate).Times(0);
    EXPECT_CALL(set_chosen_goal_candidate, set).Times(0);
    EXPECT_FALSE(res.resolve(&rl));
}

TEST_F(ResolverTest, EmptyBodyDeactivatesParentOnly) {
    EXPECT_CALL(activate_subgoals_and_candidates, activate_subgoals_and_candidates(&rl)).WillOnce(Return(true));
    EXPECT_CALL(deactivate_goal_candidates, deactivate_goal_candidates(&parent_gl)).Times(1);
    EXPECT_CALL(goal_deactivator, deactivate(&parent_gl)).Times(1);
    EXPECT_CALL(set_chosen_goal_candidate, set(&parent_gl, kRule)).Times(1);
    EXPECT_TRUE(res.resolve(&rl));
}

TEST_F(ResolverTest, SuccessDeactivatesParentCandidatesAndGoal) {
    EXPECT_CALL(activate_subgoals_and_candidates, activate_subgoals_and_candidates(&rl)).WillOnce(Return(true));
    EXPECT_CALL(deactivate_goal_candidates, deactivate_goal_candidates(&parent_gl)).Times(1);
    EXPECT_CALL(goal_deactivator, deactivate(&parent_gl)).Times(1);
    EXPECT_CALL(set_chosen_goal_candidate, set(&parent_gl, kRule)).Times(1);
    EXPECT_TRUE(res.resolve(&rl));
}
