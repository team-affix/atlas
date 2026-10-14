#include <vector>
#include <immer/map.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_body_goal_expr_descender.hpp"

namespace {

struct MockGetNodeAddedBodyGoals {
    MOCK_METHOD((const std::vector<const expr*>&), get, (pud_node_id));
};

struct MockGetCallSite {
    MOCK_METHOD(body_goal_id, get, (pud_node_id));
};

struct PudBodyGoalExprDescenderTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    ::testing::NiceMock<MockGetNodeAddedBodyGoals> get_body_goals;
    ::testing::NiceMock<MockGetCallSite> get_call_site;
    pud_body_goal_expr_descender<MockGetNodeAddedBodyGoals, MockGetCallSite> descender{
        get_body_goals,
        get_call_site};

    const expr* goal(const char* name) {
        return exprs.make_functor(functors.id(name), {});
    }
};

TEST_F(PudBodyGoalExprDescenderTest, RootDescentAssignsSequentialIds) {
    const pud_node_id root = 0;
    const expr* goal_a = goal("goal_a");
    const expr* goal_b = goal("goal_b");
    const std::vector<const expr*> goals{goal_a, goal_b};
    EXPECT_CALL(get_body_goals, get(root)).WillOnce(::testing::ReturnRef(goals));
    pud_body_goal_expr_descent descent = descender.root_descent(root);
    EXPECT_EQ(descent.body_goal_exprs.size(), 2u);
    EXPECT_EQ(descent.next_body_goal_id, 2u);
    EXPECT_EQ(*descent.body_goal_exprs.find(0), goal_a);
    EXPECT_EQ(*descent.body_goal_exprs.find(1), goal_b);
}

TEST_F(PudBodyGoalExprDescenderTest, DescendErasesParentCallSiteAndAppendsChildGoals) {
    const pud_node_id parent = 10;
    const pud_node_id child = 11;
    const expr* kept = goal("kept");
    const expr* added = goal("added");
    pud_body_goal_expr_descent at_parent{
        .body_goal_exprs = immer::map<body_goal_id, const expr*>{{0, goal("removed")}, {1, kept}},
        .next_body_goal_id = 2,
    };
    const std::vector<const expr*> child_goals{added};
    EXPECT_CALL(get_call_site, get(parent)).WillOnce(::testing::Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(::testing::ReturnRef(child_goals));
    pud_body_goal_expr_descent at_child = descender.descend(at_parent, parent, child);
    EXPECT_EQ(at_child.body_goal_exprs.count(0), 0u);
    EXPECT_EQ(at_child.body_goal_exprs.count(1), 1u);
    EXPECT_EQ(*at_child.body_goal_exprs.find(1), kept);
    EXPECT_EQ(*at_child.body_goal_exprs.find(2), added);
    EXPECT_EQ(at_child.next_body_goal_id, 3u);
}

TEST_F(PudBodyGoalExprDescenderTest, ForkedSiblingDescentsStayIndependent) {
    const pud_node_id parent = 1;
    const pud_node_id left = 2;
    const pud_node_id right = 3;
    const expr* left_goal = goal("left");
    const expr* right_goal = goal("right");
    pud_body_goal_expr_descent at_parent{
        .body_goal_exprs = immer::map<body_goal_id, const expr*>{{0, goal("call_site")}},
        .next_body_goal_id = 1,
    };
    const std::vector<const expr*> left_goals{left_goal};
    const std::vector<const expr*> right_goals{right_goal};
    EXPECT_CALL(get_call_site, get(parent)).WillRepeatedly(::testing::Return(0));
    EXPECT_CALL(get_body_goals, get(left)).WillOnce(::testing::ReturnRef(left_goals));
    pud_body_goal_expr_descent at_left = descender.descend(at_parent, parent, left);
    EXPECT_CALL(get_body_goals, get(right)).WillOnce(::testing::ReturnRef(right_goals));
    pud_body_goal_expr_descent at_right = descender.descend(at_parent, parent, right);
    EXPECT_EQ(at_left.body_goal_exprs.size(), 1u);
    EXPECT_EQ(at_right.body_goal_exprs.size(), 1u);
    EXPECT_EQ(*at_left.body_goal_exprs.find(1), left_goal);
    EXPECT_EQ(*at_right.body_goal_exprs.find(1), right_goal);
}

}
