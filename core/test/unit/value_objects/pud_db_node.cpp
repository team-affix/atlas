// pud_db_node: payload fields and ordering (lvc, unifications, body).

#include <gtest/gtest.h>
#include "value_objects/pud_db_node.hpp"
#include "value_objects/expr.hpp"

struct PudDbNodeTest : public ::testing::Test {
    PudDbNodeTest()
        : body_a_{expr::var{0}}
        , body_b_{expr::var{1}} {}

    expr body_a_;
    expr body_b_;
};

TEST_F(PudDbNodeTest, EqualWhenAllFieldsMatch) {
    const pud_db_node left{{}, {&body_a_}, 3};
    const pud_db_node right{{}, {&body_a_}, 3};
    EXPECT_EQ(left, right);
}

TEST_F(PudDbNodeTest, OrdersByLvcBeforeBody) {
    const pud_db_node early{{}, {&body_b_}, 1};
    const pud_db_node late{{}, {&body_a_}, 2};
    EXPECT_LT(early, late);
}

TEST_F(PudDbNodeTest, StoresAddedBodyGoalsAndLvc) {
    const pud_db_node node{{}, {&body_a_, &body_b_}, 7};
    EXPECT_EQ(node.lvc, 7u);
    EXPECT_EQ(node.added_body_goals.size(), 2u);
    EXPECT_EQ(node.added_body_goals[0], &body_a_);
    EXPECT_EQ(node.added_body_goals[1], &body_b_);
}
