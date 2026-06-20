// initial_goal_activator: for one initial goal index, looks up the expr, creates a
// root goal_lineage, registers the expr, and inserts into active goals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/initial_goal_activator.hpp"

using ::testing::Return;

struct MockGetInitialGoalExpr {
    MOCK_METHOD(const expr*, get, (size_t), (const));
};

struct MockMakeInitialGoalLineage {
    MOCK_METHOD(const goal_lineage*, make, (subgoal_id));
};

struct MockSetGoalExpr {
    MOCK_METHOD(void, set, (const goal_lineage*, framed_expr));
};

struct MockInsertGoalCandidates {
    MOCK_METHOD(void, insert, (const goal_lineage*));
};

struct MockInsertActiveGoal {
    MOCK_METHOD(void, insert_active_goal, (const goal_lineage*));
};

using TestInitialGoalActivator = initial_goal_activator<
    MockGetInitialGoalExpr, MockMakeInitialGoalLineage,
    MockSetGoalExpr, MockInsertGoalCandidates, MockInsertActiveGoal>;

struct InitialGoalActivatorTest : public ::testing::Test {
    static constexpr subgoal_id kIdx = 0;

    MockGetInitialGoalExpr get_initial_goal_expr;
    MockMakeInitialGoalLineage make_initial_goal_lineage;
    MockSetGoalExpr set_goal_expr;
    MockInsertGoalCandidates insert_goal_candidates;
    MockInsertActiveGoal insert_active_goal;
    TestInitialGoalActivator activator{get_initial_goal_expr, make_initial_goal_lineage,
                                       set_goal_expr, insert_goal_candidates, insert_active_goal};

    expr e0{expr::var{0}};
    goal_lineage gl0{nullptr, kIdx};
};

TEST_F(InitialGoalActivatorTest, ActivatesOneInitialGoal) {
    EXPECT_CALL(get_initial_goal_expr, get(kIdx)).WillOnce(Return(&e0));
    EXPECT_CALL(make_initial_goal_lineage, make(kIdx)).WillOnce(Return(&gl0));
    EXPECT_CALL(set_goal_expr, set(&gl0, framed_expr{&e0, 0})).Times(1);
    EXPECT_CALL(insert_goal_candidates, insert(&gl0)).Times(1);
    EXPECT_CALL(insert_active_goal, insert_active_goal(&gl0)).Times(1);

    activator.activate_initial_goal(kIdx);
}

TEST_F(InitialGoalActivatorTest, ActivatesDifferentSubgoalIndex) {
    static constexpr subgoal_id kAltIdx = 3;
    expr e3{expr::var{3}};
    goal_lineage gl3{nullptr, kAltIdx};

    EXPECT_CALL(get_initial_goal_expr, get(kAltIdx)).WillOnce(Return(&e3));
    EXPECT_CALL(make_initial_goal_lineage, make(kAltIdx)).WillOnce(Return(&gl3));
    EXPECT_CALL(set_goal_expr, set(&gl3, framed_expr{&e3, 0})).Times(1);
    EXPECT_CALL(insert_goal_candidates, insert(&gl3)).Times(1);
    EXPECT_CALL(insert_active_goal, insert_active_goal(&gl3)).Times(1);

    activator.activate_initial_goal(kAltIdx);
}
