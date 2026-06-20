// srt_goal_deactivator: erases goal candidate index bucket and unsets goal expression.
// Active-goal removal is handled separately via SRT grounding.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/srt_goal_deactivator.hpp"

struct MockUnsetGoalExpr {
    MOCK_METHOD(void, unset, (const goal_lineage*));
};

struct MockEraseGoalCandidates {
    MOCK_METHOD(void, erase, (const goal_lineage*));
};

using TestSrtGoalDeactivator = srt_goal_deactivator<MockUnsetGoalExpr, MockEraseGoalCandidates>;

struct SrtGoalDeactivatorTest : public ::testing::Test {
    MockUnsetGoalExpr unset_goal_expr;
    MockEraseGoalCandidates erase_goal_candidates;
    TestSrtGoalDeactivator deactivator{unset_goal_expr, erase_goal_candidates};

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
