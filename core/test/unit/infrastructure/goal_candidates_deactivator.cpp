// goal_candidates_deactivator: iterate parent candidates and deactivate each.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_candidates_deactivator.hpp"
#include "infrastructure/rule_id_set.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockGetGoalCandidateRuleIds {
    MOCK_METHOD(rule_id_set&, get, (const goal_lineage*));
};

struct MockMakeResolutionLineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id));
};

struct MockCandidateDeactivator {
    MOCK_METHOD(void, deactivate, (const resolution_lineage*));
};

using TestGoalCandidatesDeactivator = goal_candidates_deactivator<
    MockGetGoalCandidateRuleIds, MockMakeResolutionLineage, MockCandidateDeactivator>;

struct GoalCandidatesDeactivatorTest : public ::testing::Test {
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    MockMakeResolutionLineage make_resolution_lineage;
    MockCandidateDeactivator candidate_deactivator;
    TestGoalCandidatesDeactivator deactivator{get_goal_candidate_rule_ids,
                                              make_resolution_lineage, candidate_deactivator};

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
