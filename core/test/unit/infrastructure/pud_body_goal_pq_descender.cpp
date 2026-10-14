#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_body_goal_pq_descender.hpp"

struct MockGetNodeAddedBodyGoalsPq {
    MOCK_METHOD((const std::vector<const expr*>&), get, (pud_node_id));
};

struct MockGetCallSitePq {
    MOCK_METHOD(body_goal_id, get, (pud_node_id));
};

struct PudBodyGoalPqDescenderTest : public ::testing::Test {
    ::testing::NiceMock<MockGetNodeAddedBodyGoalsPq> get_body_goals;
    ::testing::NiceMock<MockGetCallSitePq> get_call_site;
    pud_body_goal_pq_descender<MockGetNodeAddedBodyGoalsPq, MockGetCallSitePq> descender{
        get_body_goals,
        get_call_site};
};

TEST_F(PudBodyGoalPqDescenderTest, RootDescentAssignsSequentialIds) {
    const pud_node_id root = 0;
    const std::vector<const expr*> goals{nullptr, nullptr};
    EXPECT_CALL(get_body_goals, get(root)).WillOnce(::testing::ReturnRef(goals));
    pud_body_goal_pq_descent descent = descender.root_descent(root);
    EXPECT_EQ(descent.body_goal_pqs.size(), 2u);
    EXPECT_EQ(descent.next_body_goal_id, 2u);
}

TEST_F(PudBodyGoalPqDescenderTest, DescendErasesParentCallSiteAndAppendsChildGoals) {
    const pud_node_id parent = 10;
    const pud_node_id child = 11;
    const std::vector<const expr*> root_goals{nullptr};
    EXPECT_CALL(get_body_goals, get(parent)).WillOnce(::testing::ReturnRef(root_goals));
    pud_body_goal_pq_descent at_parent = descender.root_descent(parent);
    const std::vector<const expr*> child_goals{nullptr};
    EXPECT_CALL(get_call_site, get(parent)).WillOnce(::testing::Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(::testing::ReturnRef(child_goals));
    pud_body_goal_pq_descent at_child = descender.descend(at_parent, parent, child);
    EXPECT_EQ(at_child.body_goal_pqs.count(0), 0u);
    EXPECT_EQ(at_child.body_goal_pqs.size(), 1u);
    EXPECT_EQ(at_child.next_body_goal_id, 2u);
}

TEST_F(PudBodyGoalPqDescenderTest, UntouchedGoalKeepsSameBoxIdentity) {
    const pud_node_id parent = 1;
    const pud_node_id child = 2;
    const std::vector<const expr*> root_goals{nullptr, nullptr};
    EXPECT_CALL(get_body_goals, get(parent)).WillOnce(::testing::ReturnRef(root_goals));
    pud_body_goal_pq_descent at_parent = descender.root_descent(parent);
    const persistent_query* box_before = &at_parent.body_goal_pqs.find(1)->get();
    const std::vector<const expr*> child_goals{nullptr};
    EXPECT_CALL(get_call_site, get(parent)).WillOnce(::testing::Return(0));
    EXPECT_CALL(get_body_goals, get(child)).WillOnce(::testing::ReturnRef(child_goals));
    pud_body_goal_pq_descent at_child = descender.descend(at_parent, parent, child);
    const persistent_query* box_after = &at_child.body_goal_pqs.find(1)->get();
    EXPECT_EQ(box_before, box_after);
}

TEST_F(PudBodyGoalPqDescenderTest, ForkedSiblingDescentsStayIndependent) {
    const pud_node_id parent = 1;
    const pud_node_id left = 2;
    const pud_node_id right = 3;
    const std::vector<const expr*> root_goals{nullptr};
    EXPECT_CALL(get_body_goals, get(parent)).WillOnce(::testing::ReturnRef(root_goals));
    pud_body_goal_pq_descent at_parent = descender.root_descent(parent);
    const std::vector<const expr*> left_goals{nullptr};
    const std::vector<const expr*> right_goals{nullptr};
    EXPECT_CALL(get_call_site, get(parent)).WillRepeatedly(::testing::Return(0));
    EXPECT_CALL(get_body_goals, get(left)).WillOnce(::testing::ReturnRef(left_goals));
    pud_body_goal_pq_descent at_left = descender.descend(at_parent, parent, left);
    EXPECT_CALL(get_body_goals, get(right)).WillOnce(::testing::ReturnRef(right_goals));
    pud_body_goal_pq_descent at_right = descender.descend(at_parent, parent, right);
    EXPECT_EQ(at_left.body_goal_pqs.size(), 1u);
    EXPECT_EQ(at_right.body_goal_pqs.size(), 1u);
}
