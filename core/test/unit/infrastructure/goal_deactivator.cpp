// Goal deactivator: erases goal candidate index bucket, unsets goal expression,
// and removes the goal from the active set.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include "infrastructure/goal_deactivator.hpp"
#include "interfaces/i_unset_goal_expr.hpp"
#include "interfaces/i_erase_goal_candidates.hpp"
#include "interfaces/i_erase_active_goal.hpp"

struct MockUnsetGoalExpr : public i_unset_goal_expr {
    MOCK_METHOD(void, unset, (const goal_lineage*), (override));
};

struct MockEraseGoalCandidates : public i_erase_goal_candidates {
    MOCK_METHOD(void, erase, (const goal_lineage*), (override));
};

struct MockEraseActiveGoal : public i_erase_active_goal {
    MOCK_METHOD(void, erase_active_goal, (const goal_lineage*), (override));
};

struct GoalDeactivatorTest : public ::testing::Test {
    locator loc;
    MockUnsetGoalExpr unset_goal_expr;
    MockEraseGoalCandidates erase_goal_candidates;
    MockEraseActiveGoal erase_active_goal;
    goal_deactivator deactivator;

    GoalDeactivatorTest() : deactivator(init_deactivator()) {}

    goal_deactivator init_deactivator() {
        loc.bind_as<i_unset_goal_expr>(unset_goal_expr);
        loc.bind_as<i_erase_goal_candidates>(erase_goal_candidates);
        loc.bind_as<i_erase_active_goal>(erase_active_goal);
        return goal_deactivator{loc};
    }

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
