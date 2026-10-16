#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_base_descent.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct SubDescentOps {
    MOCK_METHOD(int, descend_binding, (int, pud_node_id));
    MOCK_METHOD(int, descend_lvc, (int, pud_node_id));
    MOCK_METHOD(body_goal_id, descend_next_body_goal_id, (body_goal_id, pud_node_id));
    MOCK_METHOD(int, descend_body_goal_expr, (int, body_goal_id, pud_node_id, pud_node_id));
    MOCK_METHOD(int, descend_body_goal_pq, (int, body_goal_id, pud_node_id, pud_node_id));
};

SubDescentOps* sub_descent_ops = nullptr;

struct MockBindingDescent {
    int tag;
    MockBindingDescent descend(pud_node_id child) const {
        return {sub_descent_ops->descend_binding(tag, child)};
    }
};

struct MockLvcDescent {
    int tag;
    MockLvcDescent descend(pud_node_id child) const {
        return {sub_descent_ops->descend_lvc(tag, child)};
    }
};

struct MockNextBodyGoalIdDescent {
    body_goal_id next;
    MockNextBodyGoalIdDescent descend(pud_node_id child) const {
        return {sub_descent_ops->descend_next_body_goal_id(next, child)};
    }
    body_goal_id next_body_goal_id() const { return next; }
};

struct MockBodyGoalExprDescent {
    int tag;
    MockBodyGoalExprDescent descend(body_goal_id next_body_goal_id, pud_node_id parent, pud_node_id child) const {
        return {sub_descent_ops->descend_body_goal_expr(tag, next_body_goal_id, parent, child)};
    }
};

struct MockBodyGoalPqDescent {
    int tag;
    MockBodyGoalPqDescent descend(body_goal_id next_body_goal_id, pud_node_id parent, pud_node_id child) const {
        return {sub_descent_ops->descend_body_goal_pq(tag, next_body_goal_id, parent, child)};
    }
};

using test_descent_t = pud_base_descent<
    MockBindingDescent,
    MockLvcDescent,
    MockNextBodyGoalIdDescent,
    MockBodyGoalExprDescent,
    MockBodyGoalPqDescent>;

struct PudBaseDescentTest : public ::testing::Test {
    NiceMock<SubDescentOps> ops;
    pud_node_id parent = 1;
    pud_node_id child = 2;
    pud_node_id sibling = 3;
    test_descent_t at_parent{
        MockBindingDescent{10},
        MockLvcDescent{20},
        MockNextBodyGoalIdDescent{4},
        MockBodyGoalExprDescent{40},
        MockBodyGoalPqDescent{50},
    };

    void SetUp() override {
        sub_descent_ops = &ops;
    }

    void TearDown() override {
        sub_descent_ops = nullptr;
    }
};

TEST_F(PudBaseDescentTest, ConstructorKeepsEachRootSubDescent) {
    EXPECT_EQ(at_parent.binding().tag, 10);
    EXPECT_EQ(at_parent.lvc().tag, 20);
    EXPECT_EQ(at_parent.body_goal_expr().tag, 40);
    EXPECT_EQ(at_parent.body_goal_pq().tag, 50);
}

TEST_F(PudBaseDescentTest, DescendForwardsTheChildToEverySubDescent) {
    EXPECT_CALL(ops, descend_binding(10, child)).WillOnce(Return(11));
    EXPECT_CALL(ops, descend_lvc(20, child)).WillOnce(Return(21));
    EXPECT_CALL(ops, descend_next_body_goal_id(4, child)).WillOnce(Return(7));
    EXPECT_CALL(ops, descend_body_goal_expr(40, _, _, child)).WillOnce(Return(41));
    EXPECT_CALL(ops, descend_body_goal_pq(50, _, _, child)).WillOnce(Return(51));
    test_descent_t at_child = at_parent.descend(parent, child);
    EXPECT_EQ(at_child.binding().tag, 11);
    EXPECT_EQ(at_child.lvc().tag, 21);
    EXPECT_EQ(at_child.body_goal_expr().tag, 41);
    EXPECT_EQ(at_child.body_goal_pq().tag, 51);
}

TEST_F(PudBaseDescentTest, ExprAndPqGetTheParentsNextBodyGoalId) {
    EXPECT_CALL(ops, descend_next_body_goal_id(4, child)).WillOnce(Return(7));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 4u, _, _)).WillOnce(Return(41));
    EXPECT_CALL(ops, descend_body_goal_pq(_, 4u, _, _)).WillOnce(Return(51));
    at_parent.descend(parent, child);
}

TEST_F(PudBaseDescentTest, ExprAndPqGetTheParentNode) {
    EXPECT_CALL(ops, descend_body_goal_expr(_, _, parent, child)).WillOnce(Return(41));
    EXPECT_CALL(ops, descend_body_goal_pq(_, _, parent, child)).WillOnce(Return(51));
    at_parent.descend(parent, child);
}

TEST_F(PudBaseDescentTest, DescendLeavesParentUntouched) {
    EXPECT_CALL(ops, descend_next_body_goal_id(4, child)).WillOnce(Return(7));
    at_parent.descend(parent, child);
    EXPECT_EQ(at_parent.binding().tag, 10);
    EXPECT_CALL(ops, descend_next_body_goal_id(4, sibling)).WillOnce(Return(9));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 4u, parent, sibling)).WillOnce(Return(0));
    at_parent.descend(parent, sibling);
}

TEST_F(PudBaseDescentTest, SiblingsDescendFromTheSameParentState) {
    EXPECT_CALL(ops, descend_binding(10, child)).WillOnce(Return(11));
    EXPECT_CALL(ops, descend_binding(10, sibling)).WillOnce(Return(12));
    EXPECT_CALL(ops, descend_body_goal_expr(40, 4u, parent, child)).WillOnce(Return(41));
    EXPECT_CALL(ops, descend_body_goal_expr(40, 4u, parent, sibling)).WillOnce(Return(42));
    test_descent_t at_child = at_parent.descend(parent, child);
    test_descent_t at_sibling = at_parent.descend(parent, sibling);
    EXPECT_EQ(at_child.binding().tag, 11);
    EXPECT_EQ(at_sibling.binding().tag, 12);
    EXPECT_EQ(at_child.body_goal_expr().tag, 41);
    EXPECT_EQ(at_sibling.body_goal_expr().tag, 42);
}

TEST_F(PudBaseDescentTest, ConstructorDescendsNothing) {
    ::testing::StrictMock<SubDescentOps> strict_ops;
    sub_descent_ops = &strict_ops;
    test_descent_t descent{
        MockBindingDescent{1},
        MockLvcDescent{2},
        MockNextBodyGoalIdDescent{3},
        MockBodyGoalExprDescent{4},
        MockBodyGoalPqDescent{5},
    };
    EXPECT_EQ(descent.binding().tag, 1);
    sub_descent_ops = &ops;
}

TEST_F(PudBaseDescentTest, DescendCallsEverySubDescentExactlyOnce) {
    ::testing::StrictMock<SubDescentOps> strict_ops;
    sub_descent_ops = &strict_ops;
    EXPECT_CALL(strict_ops, descend_binding(10, child)).WillOnce(Return(0));
    EXPECT_CALL(strict_ops, descend_lvc(20, child)).WillOnce(Return(0));
    EXPECT_CALL(strict_ops, descend_next_body_goal_id(4, child)).WillOnce(Return(0));
    EXPECT_CALL(strict_ops, descend_body_goal_expr(40, 4u, parent, child)).WillOnce(Return(0));
    EXPECT_CALL(strict_ops, descend_body_goal_pq(50, 4u, parent, child)).WillOnce(Return(0));
    at_parent.descend(parent, child);
    sub_descent_ops = &ops;
}

TEST_F(PudBaseDescentTest, NextBodyGoalIdIsThreadedThroughEveryLevel) {
    const pud_node_id grandchild = 4;
    const pud_node_id great_grandchild = 5;
    EXPECT_CALL(ops, descend_next_body_goal_id(4, child)).WillOnce(Return(6));
    EXPECT_CALL(ops, descend_next_body_goal_id(6, grandchild)).WillOnce(Return(6));
    EXPECT_CALL(ops, descend_next_body_goal_id(6, great_grandchild)).WillOnce(Return(11));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 4u, parent, child)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 6u, child, grandchild)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 6u, grandchild, great_grandchild)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_pq(_, 4u, parent, child)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_pq(_, 6u, child, grandchild)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_pq(_, 6u, grandchild, great_grandchild)).WillOnce(Return(0));
    at_parent.descend(parent, child).descend(child, grandchild).descend(grandchild, great_grandchild);
}

TEST_F(PudBaseDescentTest, DescentIsAssignable) {
    EXPECT_CALL(ops, descend_binding(10, child)).WillOnce(Return(11));
    test_descent_t current = at_parent;
    current = current.descend(parent, child);
    EXPECT_EQ(current.binding().tag, 11);
}

TEST_F(PudBaseDescentTest, GrandchildUsesTheChildsNextBodyGoalId) {
    EXPECT_CALL(ops, descend_next_body_goal_id(4, child)).WillOnce(Return(7));
    test_descent_t at_child = at_parent.descend(parent, child);
    EXPECT_CALL(ops, descend_next_body_goal_id(7, sibling)).WillOnce(Return(9));
    EXPECT_CALL(ops, descend_body_goal_expr(_, 7u, child, sibling)).WillOnce(Return(0));
    EXPECT_CALL(ops, descend_body_goal_pq(_, 7u, child, sibling)).WillOnce(Return(0));
    at_child.descend(child, sibling);
}

}  // namespace
