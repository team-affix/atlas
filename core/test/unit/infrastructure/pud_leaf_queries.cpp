// pud_leaf_queries: per-leaf query records with stable heap addresses.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/pud_leaf_queries.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudLeafQueriesTest : public ::testing::Test {
    PudLeafQueriesTest()
        : open_(1)
        , close_(2)
        , interval_{om_label(&open_), om_label(&close_)}
        , body_{expr::var{0}}
        , leaf_{pud_rule_id::axiom{0}} {}

    pud_query make_query() {
        return pud_query{interval_, &body_, {}};
    }

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    pud_rule_id leaf_;
    pud_leaf_queries queries_;
};

TEST_F(PudLeafQueriesTest, ReplaceThenGetReturnsStablePointers) {
    queries_.replace_leaf_queries(&leaf_, {make_query()});
    pud_query* first = queries_.get(&leaf_).at(0);
    EXPECT_EQ(first->body_goal, &body_);
    queries_.replace_leaf_queries(&leaf_, {make_query(), make_query()});
    EXPECT_EQ(queries_.get(&leaf_).size(), 2u);
}

TEST_F(PudLeafQueriesTest, ClearRemovesTheLeaf) {
    queries_.replace_leaf_queries(&leaf_, {make_query()});
    queries_.clear_leaf_queries(&leaf_);
    EXPECT_THROW(queries_.get(&leaf_), std::out_of_range);
}
