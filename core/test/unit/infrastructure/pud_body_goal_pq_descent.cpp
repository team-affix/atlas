#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_body_goal_pq_descent.hpp"

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

using test_descent_t = pud_body_goal_pq_descent<MockGetNodeAddedBodyGoals, MockGetCallSite>;

struct PudBodyGoalPqDescentTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    NiceMock<MockGetNodeAddedBodyGoals> get_body_goals;
    NiceMock<MockGetCallSite> get_call_site;
    pud_node_id root = 0;
    pud_node_id child = 1;
    pud_node_id sibling = 2;

    const expr* goal(const char* name) {
        return exprs.make_functor(functors.id(name), {});
    }

    test_descent_t make_root(const std::vector<const expr*>& root_goals) {
        EXPECT_CALL(get_body_goals, get(root)).WillOnce(ReturnRef(root_goals));
        return test_descent_t{get_body_goals, get_call_site, root};
    }
};

TEST_F(PudBodyGoalPqDescentTest, RootAssignsIdsFromZero) {
    const std::vector<const expr*> goals{goal("g0"), goal("g1")};
    test_descent_t descent = make_root(goals);
    EXPECT_EQ(descent.body_goal_pqs().size(), 2u);
    EXPECT_NE(descent.body_goal_pqs().find(0), nullptr);
    EXPECT_NE(descent.body_goal_pqs().find(1), nullptr);
}

TEST_F(PudBodyGoalPqDescentTest, DescendErasesParentCallSiteAndAddsChildGoalsFromNextId) {
    const std::vector<const expr*> root_goals{goal("removed"), goal("kept")};
    const std::vector<const expr*> child_goals{goal("added_a"), goal("added_b")};
    test_descent_t at_root = make_root(root_goals);
    EXPECT_CALL(get_call_site, get(root)).WillOnce(Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(child_goals));
    test_descent_t at_child = at_root.descend(5, root, child);
    EXPECT_EQ(at_child.body_goal_pqs().size(), 3u);
    EXPECT_EQ(at_child.body_goal_pqs().count(0), 0u);
    EXPECT_EQ(at_child.body_goal_pqs().count(1), 1u);
    EXPECT_EQ(at_child.body_goal_pqs().count(5), 1u);
    EXPECT_EQ(at_child.body_goal_pqs().count(6), 1u);
}

TEST_F(PudBodyGoalPqDescentTest, UntouchedGoalKeepsSameBoxIdentity) {
    const std::vector<const expr*> root_goals{goal("g0"), goal("g1")};
    const std::vector<const expr*> child_goals{goal("new")};
    test_descent_t at_root = make_root(root_goals);
    const persistent_query* box_before = &at_root.body_goal_pqs().find(1)->get();
    EXPECT_CALL(get_call_site, get(root)).WillOnce(Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(child_goals));
    test_descent_t at_child = at_root.descend(2, root, child);
    EXPECT_EQ(&at_child.body_goal_pqs().find(1)->get(), box_before);
}

TEST_F(PudBodyGoalPqDescentTest, DescendLeavesParentUntouched) {
    const std::vector<const expr*> root_goals{goal("call_site")};
    const std::vector<const expr*> child_goals{goal("added")};
    test_descent_t at_root = make_root(root_goals);
    EXPECT_CALL(get_call_site, get(root)).WillOnce(Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(child_goals));
    at_root.descend(1, root, child);
    EXPECT_EQ(at_root.body_goal_pqs().size(), 1u);
    EXPECT_EQ(at_root.body_goal_pqs().count(0), 1u);
}

TEST_F(PudBodyGoalPqDescentTest, SiblingsGetDistinctBoxes) {
    const std::vector<const expr*> root_goals{goal("call_site")};
    const std::vector<const expr*> left_goals{goal("left")};
    const std::vector<const expr*> right_goals{goal("right")};
    test_descent_t at_root = make_root(root_goals);
    EXPECT_CALL(get_call_site, get(root)).WillRepeatedly(Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(ReturnRef(left_goals));
    EXPECT_CALL(get_body_goals, get(sibling)).WillOnce(ReturnRef(right_goals));
    test_descent_t at_left = at_root.descend(1, root, child);
    test_descent_t at_right = at_root.descend(1, root, sibling);
    EXPECT_EQ(at_left.body_goal_pqs().size(), 1u);
    EXPECT_EQ(at_right.body_goal_pqs().size(), 1u);
    EXPECT_NE(&at_left.body_goal_pqs().find(1)->get(), &at_right.body_goal_pqs().find(1)->get());
}

}  // namespace
