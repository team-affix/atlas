#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_axiom_initializer.hpp"
#include "infrastructure/pud_node_pool.hpp"
#include "value_objects/rule.hpp"

using ::testing::Return;

struct MockLiftExpr {
    MOCK_METHOD(const expr*, lift, (const expr*, uint32_t));
};

struct PudAxiomNodeIntegrationTest : public ::testing::Test {
    pud_node_pool pool;
    MockLiftExpr lift;
    pud_axiom_initializer<pud_node_pool, MockLiftExpr> initializer{pool, lift};
    expr head{expr::var{0}};
    expr body_a{expr::var{1}};
    expr body_b{expr::var{2}};
    expr lifted_head{expr::var{10}};
    expr lifted_a{expr::var{11}};
    expr lifted_b{expr::var{12}};
};

TEST_F(PudAxiomNodeIntegrationTest, EmptyBodyVarCountIsOneMoreAndOnlySpecIsVar0) {
    EXPECT_CALL(lift, lift(&head, 1u)).WillOnce(Return(&lifted_head));
    const pud_node* node = initializer.initialize_axiom(rule(&head, {}, 4));
    ASSERT_EQ(node->added_specializations.size(), 1u);
    EXPECT_EQ(node->added_specializations[0].var_idx, 0u);
    EXPECT_EQ(node->added_specializations[0].value, &lifted_head);
    EXPECT_TRUE(node->added_body_goals.empty());
    EXPECT_EQ(node->added_var_count, 5u);
}

TEST_F(PudAxiomNodeIntegrationTest, PayloadStillReadsBackAfterASecondAxiom) {
    EXPECT_CALL(lift, lift(&head, 1u)).WillOnce(Return(&lifted_head)).WillOnce(Return(&lifted_a));
    const pud_node* first = initializer.initialize_axiom(rule(&head, {}, 1));
    initializer.initialize_axiom(rule(&head, {}, 2));
    ASSERT_EQ(first->added_specializations.size(), 1u);
    EXPECT_EQ(first->added_specializations[0].value, &lifted_head);
    EXPECT_EQ(first->added_var_count, 2u);
}

TEST_F(PudAxiomNodeIntegrationTest, HeadAndTwoBodyGoalsLandInOrder) {
    EXPECT_CALL(lift, lift(&head, 1u)).WillOnce(Return(&lifted_head));
    EXPECT_CALL(lift, lift(&body_a, 1u)).WillOnce(Return(&lifted_a));
    EXPECT_CALL(lift, lift(&body_b, 1u)).WillOnce(Return(&lifted_b));
    const pud_node* node = initializer.initialize_axiom(rule(&head, {&body_a, &body_b}, 0));
    ASSERT_EQ(node->added_specializations.size(), 1u);
    EXPECT_EQ(node->added_specializations[0].value, &lifted_head);
    ASSERT_EQ(node->added_body_goals.size(), 2u);
    EXPECT_EQ(node->added_body_goals[0], &lifted_a);
    EXPECT_EQ(node->added_body_goals[1], &lifted_b);
}
