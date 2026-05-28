// Goal deactivator: erases goal candidate index bucket and unsets goal expression.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_deactivator.hpp"
#include "interfaces/i_unset_goal_expr.hpp"
#include "interfaces/i_erase_goal_candidates.hpp"

using ::testing::Sequence;

struct MockUnsetGoalExpr : public i_unset_goal_expr {
    MOCK_METHOD(void, unset, (const goal_lineage*), (override));
};

struct MockEraseGoalCandidates : public i_erase_goal_candidates {
    MOCK_METHOD(void, erase, (const goal_lineage*), (override));
};

struct GoalDeactivatorTest : public ::testing::Test {
    MockUnsetGoalExpr unset_goal_expr;
    MockEraseGoalCandidates erase_goal_candidates;
    goal_deactivator deactivator{unset_goal_expr, erase_goal_candidates};
    goal_lineage gl{nullptr, 0};
};

TEST_F(GoalDeactivatorTest, DeactivateErasesCandidatesThenUnsetsGoalExpr) {
    Sequence seq;
    EXPECT_CALL(erase_goal_candidates, erase(&gl)).Times(1).InSequence(seq);
    EXPECT_CALL(unset_goal_expr, unset(&gl)).Times(1).InSequence(seq);
    deactivator.deactivate(&gl);
}
