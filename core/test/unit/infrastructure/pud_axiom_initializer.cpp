#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_axiom_initializer.hpp"

using ::testing::_;
using ::testing::Return;

struct MockMakeNode {
    MOCK_METHOD(const pud_node*, make, (std::vector<pud_specialization>, std::vector<const expr*>, uint32_t));
};

struct MockLiftExpr {
    MOCK_METHOD(const expr*, lift, (const expr*, uint32_t));
};

struct PudAxiomInitializerTest : public ::testing::Test {
    MockMakeNode make_node;
    MockLiftExpr lift_expr;
    pud_axiom_initializer<MockMakeNode, MockLiftExpr> initializer{make_node, lift_expr};
    expr head{expr::var{0}};
    expr body_a{expr::var{1}};
    expr body_b{expr::var{2}};
    expr lifted_head{expr::var{3}};
    expr lifted_a{expr::var{4}};
    expr lifted_b{expr::var{5}};
    pud_node made{};
};

TEST_F(PudAxiomInitializerTest, EmptyBodyShiftsHeadAndAsksForOneMoreVar) {
    rule axiom{&head, {}, 4};
    EXPECT_CALL(lift_expr, lift(&head, 1u)).WillOnce(Return(&lifted_head));
    EXPECT_CALL(make_node, make(_, _, 5u)).WillOnce([&](std::vector<pud_specialization> specs, std::vector<const expr*> goals, uint32_t) -> const pud_node* {
        EXPECT_EQ(specs.size(), 1u);
        EXPECT_EQ(specs[0].var_idx, 0u);
        EXPECT_EQ(specs[0].value, &lifted_head);
        EXPECT_TRUE(goals.empty());
        return &made;
    });
    initializer.initialize_axiom(axiom);
}

TEST_F(PudAxiomInitializerTest, BodyGoalsAreShiftedAndKeptInOrder) {
    rule axiom{&head, {&body_a, &body_b}, 2};
    EXPECT_CALL(lift_expr, lift(&head, 1u)).WillOnce(Return(&lifted_head));
    EXPECT_CALL(lift_expr, lift(&body_a, 1u)).WillOnce(Return(&lifted_a));
    EXPECT_CALL(lift_expr, lift(&body_b, 1u)).WillOnce(Return(&lifted_b));
    EXPECT_CALL(make_node, make(_, _, 3u)).WillOnce([&](std::vector<pud_specialization>, std::vector<const expr*> goals, uint32_t) -> const pud_node* {
        EXPECT_EQ(goals.size(), 2u);
        EXPECT_EQ(goals[0], &lifted_a);
        EXPECT_EQ(goals[1], &lifted_b);
        return &made;
    });
    initializer.initialize_axiom(axiom);
}

TEST_F(PudAxiomInitializerTest, InitializeAxiomReturnsPointerMakeReturned) {
    rule axiom{&head, {&body_a, &body_b}, 6};
    EXPECT_CALL(lift_expr, lift(&head, 1u)).WillOnce(Return(&lifted_head));
    EXPECT_CALL(lift_expr, lift(&body_a, 1u)).WillOnce(Return(&lifted_a));
    EXPECT_CALL(lift_expr, lift(&body_b, 1u)).WillOnce(Return(&lifted_b));
    EXPECT_CALL(make_node, make(_, _, 7u)).WillOnce(Return(&made));
    EXPECT_EQ(initializer.initialize_axiom(axiom), &made);
}
