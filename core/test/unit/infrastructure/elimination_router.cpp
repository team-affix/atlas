// elimination_router decides whether a candidate resolution is eliminated immediately,
// deferred to the backlog, or rejected as already deactivated. Tests mock all collaborators.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/elimination_router.hpp"
#include "value_objects/elimination_result.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockRuleIdSet {
    MOCK_METHOD(bool, contains, (rule_id), (const));
};

struct MockGetGoalCandidateRuleIds {
    MOCK_METHOD(MockRuleIdSet&, get, (const goal_lineage*));
};

struct MockIsActiveGoal {
    MOCK_METHOD(bool, is_active_goal, (const goal_lineage*), (const));
};

struct MockInsertBackloggedElimination {
    MOCK_METHOD(void, insert_backlogged_elimination, (const resolution_lineage*));
};

struct MockCandidateDeactivator {
    MOCK_METHOD(void, deactivate, (const resolution_lineage*));
};

using TestEliminationRouter = elimination_router<
    MockGetGoalCandidateRuleIds, MockIsActiveGoal,
    MockInsertBackloggedElimination, MockCandidateDeactivator>;

struct EliminationRouterTest : public ::testing::Test {
    goal_lineage parent{nullptr, 0};
    resolution_lineage rl{&parent, 0};

    MockRuleIdSet rule_ids;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    MockIsActiveGoal is_active_goal;
    MockInsertBackloggedElimination insert_backlogged_elimination;
    MockCandidateDeactivator candidate_deactivator;
    TestEliminationRouter router{get_goal_candidate_rule_ids, is_active_goal,
                                 insert_backlogged_elimination, candidate_deactivator};
};

TEST_F(EliminationRouterTest, AlreadyDeactivatedReturnsAlreadyDeactivated) {
    EXPECT_CALL(is_active_goal, is_active_goal(&parent)).WillOnce(Return(true));
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&parent)).WillOnce(ReturnRef(rule_ids));
    EXPECT_CALL(rule_ids, contains(rule_id{0})).WillOnce(Return(false));
    EXPECT_CALL(insert_backlogged_elimination, insert_backlogged_elimination).Times(0);
    EXPECT_CALL(candidate_deactivator, deactivate).Times(0);

    EXPECT_EQ(router.route(&rl), elimination_result::already_deactivated);
}

TEST_F(EliminationRouterTest, InactiveParentAddsToBacklog) {
    EXPECT_CALL(is_active_goal, is_active_goal(&parent)).WillOnce(Return(false));
    EXPECT_CALL(insert_backlogged_elimination, insert_backlogged_elimination(&rl)).Times(1);
    EXPECT_CALL(candidate_deactivator, deactivate).Times(0);

    EXPECT_EQ(router.route(&rl), elimination_result::added_to_backlog);
}

TEST_F(EliminationRouterTest, ActiveParentEliminatesCandidate) {
    EXPECT_CALL(is_active_goal, is_active_goal(&parent)).WillOnce(Return(true));
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&parent)).WillOnce(ReturnRef(rule_ids));
    EXPECT_CALL(rule_ids, contains(rule_id{0})).WillOnce(Return(true));
    EXPECT_CALL(candidate_deactivator, deactivate(&rl)).Times(1);
    EXPECT_CALL(insert_backlogged_elimination, insert_backlogged_elimination).Times(0);

    EXPECT_EQ(router.route(&rl), elimination_result::eliminated);
}

TEST_F(EliminationRouterTest, BackloggedRuleNotInSetReturnsAlreadyDeactivated) {
    EXPECT_CALL(is_active_goal, is_active_goal(&parent)).WillOnce(Return(true));
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&parent)).WillOnce(ReturnRef(rule_ids));
    EXPECT_CALL(rule_ids, contains(rule_id{0})).WillOnce(Return(false));
    EXPECT_CALL(candidate_deactivator, deactivate).Times(0);

    EXPECT_EQ(router.route(&rl), elimination_result::already_deactivated);
}
