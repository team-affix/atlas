// pud_db_node: payload fields and ordering (lvc, unifications, body, interval ranks).

#include <gtest/gtest.h>
#include "value_objects/pud_db_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"

struct PudDbNodeTest : public ::testing::Test {
    PudDbNodeTest()
        : open_a_(1)
        , close_a_(2)
        , open_b_(3)
        , close_b_(4)
        , interval_a_{om_label(&open_a_), om_label(&close_a_)}
        , interval_b_{om_label(&open_b_), om_label(&close_b_)}
        , body_a_{expr::var{0}}
        , body_b_{expr::var{1}} {}

    uint64_t open_a_;
    uint64_t close_a_;
    uint64_t open_b_;
    uint64_t close_b_;
    om_interval interval_a_;
    om_interval interval_b_;
    expr body_a_;
    expr body_b_;
};

TEST_F(PudDbNodeTest, EqualWhenAllFieldsMatch) {
    const pud_db_node left{interval_a_, {}, {&body_a_}, 3};
    const pud_db_node right{interval_a_, {}, {&body_a_}, 3};
    EXPECT_EQ(left, right);
}

TEST_F(PudDbNodeTest, OrdersByLvcBeforeBody) {
    const pud_db_node early{interval_a_, {}, {&body_b_}, 1};
    const pud_db_node late{interval_a_, {}, {&body_a_}, 2};
    EXPECT_LT(early, late);
}

TEST_F(PudDbNodeTest, StoresAddedBodyGoalsAndLvc) {
    const pud_db_node node{interval_a_, {}, {&body_a_, &body_b_}, 7};
    EXPECT_EQ(node.lvc, 7u);
    EXPECT_EQ(node.added_body_goals.size(), 2u);
    EXPECT_EQ(node.added_body_goals[0], &body_a_);
    EXPECT_EQ(node.added_body_goals[1], &body_b_);
}
