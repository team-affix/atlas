#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/elimination_router.hpp"
#include "../../../core/hpp/value_objects/elimination_result.hpp"
#include "../../../core/hpp/interfaces/i_deactivated_candidate_memory.hpp"
#include "../../../core/hpp/interfaces/i_is_active_goal.hpp"
#include "../../../core/hpp/interfaces/i_elimination_backlog.hpp"
#include "../../../core/hpp/interfaces/i_candidate_deactivator.hpp"

using ::testing::Return;

struct MockDeactivatedCandidateMemory : public i_deactivated_candidate_memory {
    MOCK_METHOD(void, insert, (const resolution_lineage*), (override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(bool, contains, (const resolution_lineage*), (const, override));
};

struct MockIsActiveGoal : public i_is_active_goal {
    MOCK_METHOD(bool, is_active_goal, (const goal_lineage*), (const, override));
};

struct MockEliminationBacklog : public i_elimination_backlog {
    MOCK_METHOD(void, insert, (const resolution_lineage*), (override));
    MOCK_METHOD(bool, contains, (const resolution_lineage*), (override));
    MOCK_METHOD(void, constrain, (const resolution_lineage*), (override));
};

struct MockCandidateDeactivator : public i_candidate_deactivator {
    MOCK_METHOD(void, deactivate, (const resolution_lineage*), (override));
};

struct EliminationRouterTest : public ::testing::Test {
    expr goal_e{expr::var{0}};
    expr head{expr::var{1}};
    rule idx{&head, {}};
    goal_lineage parent{nullptr, &goal_e};
    resolution_lineage rl{&parent, &idx};

    MockDeactivatedCandidateMemory dcm;
    MockIsActiveGoal is_active_goal;
    MockEliminationBacklog eb;
    MockCandidateDeactivator cd;
    elimination_router router{dcm, is_active_goal, eb, cd};
};

TEST_F(EliminationRouterTest, AlreadyDeactivatedReturnsAlreadyDeactivated) {
    EXPECT_CALL(dcm, contains(&rl)).WillOnce(Return(true));
    EXPECT_CALL(eb, insert).Times(0);
    EXPECT_CALL(cd, deactivate).Times(0);

    EXPECT_EQ(router.route(&rl), elimination_result::already_deactivated);
}

TEST_F(EliminationRouterTest, InactiveParentAddsToBacklog) {
    EXPECT_CALL(dcm, contains(&rl)).WillOnce(Return(false));
    EXPECT_CALL(is_active_goal, is_active_goal(&parent)).WillOnce(Return(false));
    EXPECT_CALL(eb, insert(&rl)).Times(1);
    EXPECT_CALL(cd, deactivate).Times(0);

    EXPECT_EQ(router.route(&rl), elimination_result::added_to_backlog);
}

TEST_F(EliminationRouterTest, ActiveParentEliminatesCandidate) {
    EXPECT_CALL(dcm, contains(&rl)).WillOnce(Return(false));
    EXPECT_CALL(is_active_goal, is_active_goal(&parent)).WillOnce(Return(true));
    EXPECT_CALL(cd, deactivate(&rl)).Times(1);
    EXPECT_CALL(eb, insert).Times(0);

    EXPECT_EQ(router.route(&rl), elimination_result::eliminated);
}
