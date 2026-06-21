// Goal deactivator: erases goal candidate index bucket, unsets goal expression,
// and removes the goal from the active set.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_deactivator.hpp"

struct MockUnsetGoalExpr {
    MOCK_METHOD(void, unset, (const goal_lineage*));
};

struct MockEraseGoalCandidates {
    MOCK_METHOD(void, erase, (const goal_lineage*));
};

struct MockEraseActiveGoal {
    MOCK_METHOD(void, erase_active_goal, (const goal_lineage*));
};

using test_goal_deactivator_t = goal_deactivator<MockUnsetGoalExpr, MockEraseGoalCandidates, MockEraseActiveGoal>;

struct GoalDeactivatorTest : public ::testing::Test {
    MockUnsetGoalExpr unset_goal_expr;
    MockEraseGoalCandidates erase_goal_candidates;
    MockEraseActiveGoal erase_active_goal;
    test_goal_deactivator_t deactivator{unset_goal_expr, erase_goal_candidates, erase_active_goal};

    goal_lineage gl{nullptr, 0};
};

TEST_F(GoalDeactivatorTest, DeactivateErasesCandidatesUnsetsExprAndActiveGoal) {
    bool erased_candidates = false;
    bool unset_expr = false;
    bool erased_active = false;
    EXPECT_CALL(erase_goal_candidates, erase(&gl))
        .WillOnce([&] { erased_candidates = true; });
    EXPECT_CALL(unset_goal_expr, unset(&gl))
        .WillOnce([&] { unset_expr = true; });
    EXPECT_CALL(erase_active_goal, erase_active_goal(&gl))
        .WillOnce([&] { erased_active = true; });
    deactivator.deactivate(&gl);
    EXPECT_TRUE(erased_candidates);
    EXPECT_TRUE(unset_expr);
    EXPECT_TRUE(erased_active);
}

TEST_F(GoalDeactivatorTest, DeactivateUsesGivenGoalLineage) {
    resolution_lineage parent{nullptr, 1};
    goal_lineage child{&parent, 2};
    bool erased_candidates = false;
    bool unset_expr = false;
    bool erased_active = false;
    EXPECT_CALL(erase_goal_candidates, erase(&child))
        .WillOnce([&] { erased_candidates = true; });
    EXPECT_CALL(unset_goal_expr, unset(&child))
        .WillOnce([&] { unset_expr = true; });
    EXPECT_CALL(erase_active_goal, erase_active_goal(&child))
        .WillOnce([&] { erased_active = true; });
    deactivator.deactivate(&child);
    EXPECT_TRUE(erased_candidates);
    EXPECT_TRUE(unset_expr);
    EXPECT_TRUE(erased_active);
}

TEST_F(GoalDeactivatorTest, SecondDeactivateStillInvokesAllCollaborators) {
    EXPECT_CALL(erase_goal_candidates, erase(&gl)).Times(2);
    EXPECT_CALL(unset_goal_expr, unset(&gl)).Times(2);
    EXPECT_CALL(erase_active_goal, erase_active_goal(&gl)).Times(2);
    deactivator.deactivate(&gl);
    deactivator.deactivate(&gl);
}
