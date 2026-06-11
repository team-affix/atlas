// srt_goal_deactivator: erases goal candidate index bucket and unsets goal expression.
// Active-goal removal is handled separately via SRT grounding.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/srt_goal_deactivator.hpp"
#include "interfaces/i_unset_goal_expr.hpp"
#include "interfaces/i_erase_goal_candidates.hpp"

struct MockUnsetGoalExpr : public i_unset_goal_expr {
    MOCK_METHOD(void, unset, (const goal_lineage*), (override));
};

struct MockEraseGoalCandidates : public i_erase_goal_candidates {
    MOCK_METHOD(void, erase, (const goal_lineage*), (override));
};

struct SrtGoalDeactivatorTest : public ::testing::Test {
    locator loc;
    MockUnsetGoalExpr unset_goal_expr;
    MockEraseGoalCandidates erase_goal_candidates;
    srt_goal_deactivator deactivator;

    SrtGoalDeactivatorTest() : deactivator(init_deactivator()) {}

    srt_goal_deactivator init_deactivator() {
        loc.bind_as<i_unset_goal_expr>(unset_goal_expr);
        loc.bind_as<i_erase_goal_candidates>(erase_goal_candidates);
        return srt_goal_deactivator{loc};
    }

    goal_lineage gl{nullptr, 0};
};

TEST_F(SrtGoalDeactivatorTest, DeactivateErasesCandidatesAndUnsetsExpr) {
    bool erased_candidates = false;
    bool unset_expr = false;
    EXPECT_CALL(erase_goal_candidates, erase(&gl))
        .WillOnce([&] { erased_candidates = true; });
    EXPECT_CALL(unset_goal_expr, unset(&gl))
        .WillOnce([&] { unset_expr = true; });
    deactivator.deactivate(&gl);
    EXPECT_TRUE(erased_candidates);
    EXPECT_TRUE(unset_expr);
}

TEST_F(SrtGoalDeactivatorTest, DeactivateUsesGivenGoalLineage) {
    resolution_lineage parent{nullptr, 1};
    goal_lineage child{&parent, 2};
    bool erased_candidates = false;
    bool unset_expr = false;
    EXPECT_CALL(erase_goal_candidates, erase(&child))
        .WillOnce([&] { erased_candidates = true; });
    EXPECT_CALL(unset_goal_expr, unset(&child))
        .WillOnce([&] { unset_expr = true; });
    deactivator.deactivate(&child);
    EXPECT_TRUE(erased_candidates);
    EXPECT_TRUE(unset_expr);
}

TEST_F(SrtGoalDeactivatorTest, SecondDeactivateStillInvokesAllCollaborators) {
    EXPECT_CALL(erase_goal_candidates, erase(&gl)).Times(2);
    EXPECT_CALL(unset_goal_expr, unset(&gl)).Times(2);
    deactivator.deactivate(&gl);
    deactivator.deactivate(&gl);
}
