// initial_goal_activator: for one initial goal index, looks up the expr, creates a
// root goal_lineage, registers the expr, and inserts into active goals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/initial_goal_activator.hpp"
#include "interfaces/i_get_initial_goal_expr.hpp"
#include "interfaces/i_make_initial_goal_lineage.hpp"
#include "interfaces/i_set_goal_expr.hpp"
#include "interfaces/i_insert_active_goal.hpp"

using ::testing::Return;

struct MockGetInitialGoalExpr : public i_get_initial_goal_expr {
    MOCK_METHOD(const expr*, get, (size_t), (const, override));
};

struct MockMakeInitialGoalLineage : public i_make_initial_goal_lineage {
    MOCK_METHOD(const goal_lineage*, make, (subgoal_id), (override));
};

struct MockSetGoalExpr : public i_set_goal_expr {
    MOCK_METHOD(void, set, (const goal_lineage*, const expr*), (override));
};

struct MockInsertActiveGoal : public i_insert_active_goal {
    MOCK_METHOD(void, insert_active_goal, (const goal_lineage*), (override));
};

struct InitialGoalActivatorTest : public ::testing::Test {
    static constexpr subgoal_id kIdx = 0;

    MockGetInitialGoalExpr get_initial_goal_expr;
    MockMakeInitialGoalLineage make_initial_goal_lineage;
    MockSetGoalExpr set_goal_expr;
    MockInsertActiveGoal insert_active_goal;
    initial_goal_activator activator{
        get_initial_goal_expr,
        make_initial_goal_lineage,
        set_goal_expr,
        insert_active_goal};

    expr e0{expr::var{0}};
    goal_lineage gl0{nullptr, kIdx};
};

TEST_F(InitialGoalActivatorTest, ActivatesOneInitialGoal) {
    EXPECT_CALL(get_initial_goal_expr, get(kIdx)).WillOnce(Return(&e0));
    EXPECT_CALL(make_initial_goal_lineage, make(kIdx)).WillOnce(Return(&gl0));
    EXPECT_CALL(set_goal_expr, set(&gl0, &e0)).Times(1);
    EXPECT_CALL(insert_active_goal, insert_active_goal(&gl0)).Times(1);

    activator.activate_initial_goal(kIdx);
}
