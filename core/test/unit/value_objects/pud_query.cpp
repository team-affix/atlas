// pud_query: leaf-owned search record with interval, body, and per-axiom contexts.

#include <gtest/gtest.h>
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudQueryTest : public ::testing::Test {
    PudQueryTest()
        : open_(1)
        , close_(2)
        , interval_{om_label(&open_), om_label(&close_)}
        , body_{expr::var{0}}
        , axiom_{pud_rule_id::axiom{0}} {}

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    pud_rule_id axiom_;
};

TEST_F(PudQueryTest, EqualQueriesCompareEqual) {
    const pud_query left{interval_, &body_, {pud_candidate_search_context{&axiom_, {}}}};
    const pud_query right{interval_, &body_, {pud_candidate_search_context{&axiom_, {}}}};
    EXPECT_EQ(left, right);
}

TEST_F(PudQueryTest, DifferentBodyPointersCompareUnequal) {
    expr other{expr::var{1}};
    const pud_query left{interval_, &body_, {}};
    const pud_query right{interval_, &other, {}};
    EXPECT_NE(left, right);
}
