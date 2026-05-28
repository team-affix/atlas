// Goal deactivator: erases goal candidate index bucket and unsets goal expression.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
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
    locator loc;
    MockUnsetGoalExpr unset_goal_expr;
    MockEraseGoalCandidates erase_goal_candidates;
    goal_deactivator deactivator;

    GoalDeactivatorTest() : deactivator(init_deactivator()) {}

    goal_deactivator init_deactivator() {
        loc.bind_as<i_unset_goal_expr>(unset_goal_expr);
        loc.bind_as<i_erase_goal_candidates>(erase_goal_candidates);
        return goal_deactivator{loc};
    }
    goal_lineage gl{nullptr, 0};
};

TEST_F(GoalDeactivatorTest, DeactivateErasesCandidatesThenUnsetsGoalExpr) {
    Sequence seq;
    EXPECT_CALL(erase_goal_candidates, erase(&gl)).Times(1).InSequence(seq);
    EXPECT_CALL(unset_goal_expr, unset(&gl)).Times(1).InSequence(seq);
    deactivator.deactivate(&gl);
}
