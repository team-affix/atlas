// elimination_router decides whether a candidate resolution is eliminated immediately,
// deferred to the backlog, or rejected as already deactivated. Tests mock all four
// collaborators and assert the elimination_result for each branch.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/elimination_router.hpp"
#include "value_objects/elimination_result.hpp"
#include "interfaces/i_deactivated_candidate_memory.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_insert_backlogged_elimination.hpp"
#include "interfaces/i_candidate_deactivator.hpp"

using ::testing::Return;

struct MockDeactivatedCandidateMemory : public i_deactivated_candidate_memory {
    MOCK_METHOD(void, insert, (const resolution_lineage*), (override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(bool, contains, (const resolution_lineage*), (const, override));
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
    expr goal_e{expr::var{0}};
    expr head{expr::var{1}};
    rule idx{&head, {}};
    goal_lineage parent{nullptr, 0};
    resolution_lineage rl{&parent, 0};

    MockDeactivatedCandidateMemory dcm;
    MockIsActiveGoal is_active_goal;
    MockInsertBackloggedElimination insert_backlogged_elimination;
    MockCandidateDeactivator candidate_deactivator;
    elimination_router router{dcm, is_active_goal, insert_backlogged_elimination, candidate_deactivator};
};

TEST_F(EliminationRouterTest, AlreadyDeactivatedReturnsAlreadyDeactivated) {
    EXPECT_CALL(dcm, contains(&rl)).WillOnce(Return(true));
    EXPECT_CALL(insert_backlogged_elimination, insert_backlogged_elimination).Times(0);
    EXPECT_CALL(candidate_deactivator, deactivate).Times(0);

    EXPECT_EQ(router.route(&rl), elimination_result::already_deactivated);
}

TEST_F(EliminationRouterTest, InactiveParentAddsToBacklog) {
    EXPECT_CALL(dcm, contains(&rl)).WillOnce(Return(false));
    EXPECT_CALL(is_active_goal, is_active_goal(&parent)).WillOnce(Return(false));
    EXPECT_CALL(insert_backlogged_elimination, insert_backlogged_elimination(&rl)).Times(1);
    EXPECT_CALL(candidate_deactivator, deactivate).Times(0);

    EXPECT_EQ(router.route(&rl), elimination_result::added_to_backlog);
}

TEST_F(EliminationRouterTest, ActiveParentEliminatesCandidate) {
    EXPECT_CALL(dcm, contains(&rl)).WillOnce(Return(false));
    EXPECT_CALL(is_active_goal, is_active_goal(&parent)).WillOnce(Return(true));
    EXPECT_CALL(candidate_deactivator, deactivate(&rl)).Times(1);
    EXPECT_CALL(insert_backlogged_elimination, insert_backlogged_elimination).Times(0);

    EXPECT_EQ(router.route(&rl), elimination_result::eliminated);
}
