#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_body_goal_expr_descent.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeAddedBodyGoals {
    MOCK_METHOD((const std::vector<const expr*>&), get, (pud_node_id));
};

struct MockGetCallSite {
    MOCK_METHOD(body_goal_id, get, (pud_node_id));
};

using test_descent_t = pud_body_goal_expr_descent<MockGetNodeAddedBodyGoals, MockGetCallSite>;

struct PudBodyGoalExprDescentTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    NiceMock<MockGetNodeAddedBodyGoals> get_body_goals;
    NiceMock<MockGetCallSite> get_call_site;
    pud_node_id root = 0;
    pud_node_id child = 1;
    pud_node_id sibling = 2;
    pud_node_id grandchild = 3;

    const expr* goal(const char* name) {
        return exprs.make_functor(functors.id(name), {});
    }

    test_descent_t make_root(const std::vector<const expr*>& root_goals) {
        EXPECT_CALL(get_body_goals, get(root)).WillOnce(ReturnRef(root_goals));
        return test_descent_t{get_body_goals, get_call_site, root};
    }
};

TEST_F(PudBodyGoalExprDescentTest, RootAssignsIdsFromZero) {
    const expr* goal_a = goal("goal_a");
    const expr* goal_b = goal("goal_b");
    const std::vector<const expr*> goals{goal_a, goal_b};
    test_descent_t descent = make_root(goals);
    EXPECT_EQ(descent.body_goal_exprs().size(), 2u);
    EXPECT_EQ(*descent.body_goal_exprs().find(0), goal_a);
    EXPECT_EQ(*descent.body_goal_exprs().find(1), goal_b);
}

TEST_F(PudBodyGoalExprDescentTest, RootWithNoGoalsIsEmpty) {
    const std::vector<const expr*> goals{};
    EXPECT_TRUE(make_root(goals).body_goal_exprs().empty());
}

TEST_F(PudBodyGoalExprDescentTest, DescendErasesParentCallSiteAndAddsChildGoalsFromNextId) {
    const expr* kept = goal("kept");
    const expr* added_a = goal("added_a");
    const expr* added_b = goal("added_b");
    const std::vector<const expr*> root_goals{goal("removed"), kept};
    const std::vector<const expr*> child_goals{added_a, added_b};
    test_descent_t at_root = make_root(root_goals);
    EXPECT_CALL(get_call_site, get(root)).WillOnce(Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(child_goals));
    test_descent_t at_child = at_root.descend(5, root, child);
    EXPECT_EQ(at_child.body_goal_exprs().size(), 3u);
    EXPECT_EQ(at_child.body_goal_exprs().count(0), 0u);
    EXPECT_EQ(*at_child.body_goal_exprs().find(1), kept);
    EXPECT_EQ(*at_child.body_goal_exprs().find(5), added_a);
    EXPECT_EQ(*at_child.body_goal_exprs().find(6), added_b);
}

TEST_F(PudBodyGoalExprDescentTest, FactChildOnlyErasesTheCallSite) {
    const expr* kept = goal("kept");
    const std::vector<const expr*> root_goals{goal("removed"), kept};
    const std::vector<const expr*> no_goals{};
    test_descent_t at_root = make_root(root_goals);
    EXPECT_CALL(get_call_site, get(root)).WillOnce(Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(no_goals));
    test_descent_t at_child = at_root.descend(2, root, child);
    EXPECT_EQ(at_child.body_goal_exprs().size(), 1u);
    EXPECT_EQ(*at_child.body_goal_exprs().find(1), kept);
}

TEST_F(PudBodyGoalExprDescentTest, DescendLeavesParentUntouched) {
    const std::vector<const expr*> root_goals{goal("call_site")};
    const std::vector<const expr*> child_goals{goal("added")};
    test_descent_t at_root = make_root(root_goals);
    EXPECT_CALL(get_call_site, get(root)).WillOnce(Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(child_goals));
    at_root.descend(1, root, child);
    EXPECT_EQ(at_root.body_goal_exprs().size(), 1u);
    EXPECT_EQ(at_root.body_goal_exprs().count(0), 1u);
}

TEST_F(PudBodyGoalExprDescentTest, SiblingsDescendIndependently) {
    const expr* left_goal = goal("left");
    const expr* right_goal = goal("right");
    const std::vector<const expr*> root_goals{goal("call_site")};
    const std::vector<const expr*> left_goals{left_goal};
    const std::vector<const expr*> right_goals{right_goal};
    test_descent_t at_root = make_root(root_goals);
    EXPECT_CALL(get_call_site, get(root)).WillRepeatedly(Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(left_goals));
    EXPECT_CALL(get_body_goals, get(sibling)).WillOnce(ReturnRef(right_goals));
    test_descent_t at_left = at_root.descend(1, root, child);
    test_descent_t at_right = at_root.descend(1, root, sibling);
    EXPECT_EQ(*at_left.body_goal_exprs().find(1), left_goal);
    EXPECT_EQ(*at_right.body_goal_exprs().find(1), right_goal);
}

TEST_F(PudBodyGoalExprDescentTest, CallSiteIsLookedUpOnTheParent) {
    const std::vector<const expr*> root_goals{goal("g0"), goal("g1")};
    const std::vector<const expr*> child_goals{goal("c0")};
    const std::vector<const expr*> no_goals{};
    test_descent_t at_root = make_root(root_goals);
    EXPECT_CALL(get_call_site, get(root)).WillOnce(Return(1));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(child_goals));
    test_descent_t at_child = at_root.descend(2, root, child);
    EXPECT_CALL(get_call_site, get(child)).WillOnce(Return(2));
    EXPECT_CALL(get_body_goals, get(grandchild)).WillOnce(ReturnRef(no_goals));
    test_descent_t at_grandchild = at_child.descend(3, child, grandchild);
    EXPECT_EQ(at_grandchild.body_goal_exprs().size(), 1u);
    EXPECT_EQ(at_grandchild.body_goal_exprs().count(0), 1u);
}

}  // namespace
