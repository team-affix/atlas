// Goal deactivator: delegates unset of goal expression on deactivate. Each deactivate
// must target exactly the lineage passed in.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/goal_deactivator.hpp"
#include "../../../core/hpp/interfaces/i_unset_goal_expr.hpp"

struct MockUnsetGoalExpr : public i_unset_goal_expr {
    MOCK_METHOD(void, unset, (const goal_lineage*), (override));
};

struct GoalDeactivatorTest : public ::testing::Test {
    MockUnsetGoalExpr unset_goal_expr;
    goal_deactivator deactivator{unset_goal_expr};
    expr e{expr::var{0}};
    goal_lineage gl{nullptr, &e};
};

TEST_F(GoalDeactivatorTest, DeactivateUnsetsGoalExpr) {
    EXPECT_CALL(unset_goal_expr, unset(&gl)).Times(1);
    deactivator.deactivate(&gl);
}
