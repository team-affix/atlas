// Resolver: delegates subgoal activation, then deactivates parent candidates and goal expr.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_candidate_deactivator.hpp"
#include "interfaces/i_activate_subgoals.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id), (override));
};

struct MockGoalDeactivator : public i_goal_deactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*), (override));
};

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_id_set&, get, (const goal_lineage*), (const, override));
};

struct MockCandidateDeactivator : public i_candidate_deactivator {
    MOCK_METHOD(void, deactivate, (const resolution_lineage*), (override));
};

struct MockActivateSubgoals : public i_activate_subgoals {
    MOCK_METHOD(bool, activate_subgoals, (const resolution_lineage*), (override));
};

struct ResolverTest : public ::testing::Test {
    locator loc;
    MockMakeResolutionLineage make_resolution_lineage;
    MockGoalDeactivator goal_deactivator;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    MockCandidateDeactivator candidate_deactivator;
    MockActivateSubgoals activate_subgoals;
    resolver res;

    ResolverTest() : res(init_resolver()) {}

    resolver init_resolver() {
        loc.bind_as<i_make_resolution_lineage>(make_resolution_lineage);
        loc.bind_as<i_goal_deactivator>(goal_deactivator);
        loc.bind_as<i_get_goal_candidate_rule_ids>(get_goal_candidate_rule_ids);
        loc.bind_as<i_candidate_deactivator>(candidate_deactivator);
        loc.bind_as<i_activate_subgoals>(activate_subgoals);
        return resolver{loc};
    }

    static constexpr rule_id kRule = 0;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, kRule};
    rule_id_set parent_candidates;
};

TEST_F(ResolverTest, ReturnsFalseWhenSubgoalsFail) {
    EXPECT_CALL(activate_subgoals, activate_subgoals(&rl)).WillOnce(Return(false));
    EXPECT_CALL(goal_deactivator, deactivate).Times(0);
    EXPECT_FALSE(res.resolve(&rl));
}

TEST_F(ResolverTest, EmptyBodyDeactivatesParentOnly) {
    EXPECT_CALL(activate_subgoals, activate_subgoals(&rl)).WillOnce(Return(true));
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&parent_gl)).WillOnce(ReturnRef(parent_candidates));
    EXPECT_CALL(goal_deactivator, deactivate(&parent_gl)).Times(1);
    EXPECT_TRUE(res.resolve(&rl));
}

TEST_F(ResolverTest, SuccessDeactivatesParentCandidatesWithoutUnitPush) {
    static constexpr rule_id kParentCand = 7;
    parent_candidates.insert(kParentCand);
    resolution_lineage cand_rl{&parent_gl, kParentCand};

    EXPECT_CALL(activate_subgoals, activate_subgoals(&rl)).WillOnce(Return(true));
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&parent_gl)).WillOnce(ReturnRef(parent_candidates));
    EXPECT_CALL(make_resolution_lineage, make_resolution_lineage(&parent_gl, kParentCand))
        .WillOnce(Return(&cand_rl));
    EXPECT_CALL(candidate_deactivator, deactivate(&cand_rl)).Times(1);
    EXPECT_CALL(goal_deactivator, deactivate(&parent_gl)).Times(1);

    EXPECT_TRUE(res.resolve(&rl));
}
