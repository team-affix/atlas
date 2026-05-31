// elimination_router decides whether a candidate resolution is eliminated immediately,
// deferred to the backlog, or rejected as already deactivated. Tests mock all collaborators.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/elimination_router.hpp"
#include "value_objects/elimination_result.hpp"
#include "interfaces/i_rule_id_set.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_insert_backlogged_elimination.hpp"
#include "interfaces/i_candidate_deactivator.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockRuleIdSet : public i_rule_id_set {
    MOCK_METHOD(void, insert, (rule_id), (override));
    MOCK_METHOD(void, erase, (rule_id), (override));
    MOCK_METHOD(bool, contains, (rule_id), (const, override));
    MOCK_METHOD((coroutine<rule_id, void>), iterate, (), (const, override));
    MOCK_METHOD(size_t, size, (), (const, override));
    MOCK_METHOD(std::unique_ptr<i_rule_id_set>, copy, (), (const, override));
};

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_id_set&, get, (const goal_lineage*), (const, override));
};

struct MockIsActiveGoal : public i_is_active_goal {
    MOCK_METHOD(bool, is_active_goal, (const goal_lineage*), (const, override));
};

struct MockInsertBackloggedElimination : public i_insert_backlogged_elimination {
    MOCK_METHOD(void, insert_backlogged_elimination, (const resolution_lineage*), (override));
};

struct MockCandidateDeactivator : public i_candidate_deactivator {
    MOCK_METHOD(void, deactivate, (const resolution_lineage*), (override));
};

struct EliminationRouterTest : public ::testing::Test {
    goal_lineage parent{nullptr, 0};
    resolution_lineage rl{&parent, 0};

    locator loc;
    MockRuleIdSet rule_ids;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    MockIsActiveGoal is_active_goal;
    MockInsertBackloggedElimination insert_backlogged_elimination;
    MockCandidateDeactivator candidate_deactivator;
    elimination_router router;

    EliminationRouterTest() : router(init_router()) {}

    elimination_router init_router() {
        loc.bind_as<i_get_goal_candidate_rule_ids>(get_goal_candidate_rule_ids);
        loc.bind_as<i_is_active_goal>(is_active_goal);
        loc.bind_as<i_insert_backlogged_elimination>(insert_backlogged_elimination);
        loc.bind_as<i_candidate_deactivator>(candidate_deactivator);
        return elimination_router{loc};
    }
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
