// goal_candidates_deactivator: iterate parent candidates and deactivate each.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/goal_candidates_deactivator.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_candidate_deactivator.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_id_set&, get, (const goal_lineage*), (const, override));
};

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id), (override));
};

struct MockCandidateDeactivator : public i_candidate_deactivator {
    MOCK_METHOD(void, deactivate, (const resolution_lineage*), (override));
};

struct GoalCandidatesDeactivatorTest : public ::testing::Test {
    locator loc;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    MockMakeResolutionLineage make_resolution_lineage;
    MockCandidateDeactivator candidate_deactivator;
    goal_candidates_deactivator deactivator;

    GoalCandidatesDeactivatorTest() : deactivator(init_deactivator()) {}

    goal_candidates_deactivator init_deactivator() {
        loc.bind_as<i_get_goal_candidate_rule_ids>(get_goal_candidate_rule_ids);
        loc.bind_as<i_make_resolution_lineage>(make_resolution_lineage);
        loc.bind_as<i_candidate_deactivator>(candidate_deactivator);
        return goal_candidates_deactivator{loc};
    }

    goal_lineage gl{nullptr, 0};
    rule_id_set parent_candidates;
};

TEST_F(GoalCandidatesDeactivatorTest, EmptyCandidatesDoesNothing) {
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(parent_candidates));
    EXPECT_CALL(candidate_deactivator, deactivate).Times(0);
    deactivator.deactivate_goal_candidates(&gl);
}

TEST_F(GoalCandidatesDeactivatorTest, DeactivatesEachParentCandidate) {
    static constexpr rule_id kParentCand = 7;
    parent_candidates.insert(kParentCand);
    resolution_lineage cand_rl{&gl, kParentCand};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(parent_candidates));
    EXPECT_CALL(make_resolution_lineage, make_resolution_lineage(&gl, kParentCand))
        .WillOnce(Return(&cand_rl));
    EXPECT_CALL(candidate_deactivator, deactivate(&cand_rl)).Times(1);
    deactivator.deactivate_goal_candidates(&gl);
}
