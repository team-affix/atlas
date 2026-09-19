// pud_witness_watchers: reverse index from witness node to watching queries.

#include <gtest/gtest.h>
#include "infrastructure/pud_witness_watchers.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudWitnessWatchersTest : public ::testing::Test {
    PudWitnessWatchersTest()
        : open_(1)
        , close_(2)
        , interval_{om_label(&open_), om_label(&close_)}
        , body_{expr::var{0}}
        , witness_{pud_rule_id::axiom{0}}
        , other_{pud_rule_id::axiom{1}}
        , query_{interval_, &body_, {}} {}

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    pud_rule_id witness_;
    pud_rule_id other_;
    pud_query query_;
    pud_witness_watchers watchers_;
};

TEST_F(PudWitnessWatchersTest, WatchThenGetReturnsTheQuery) {
    watchers_.watch(&witness_, &query_);
    EXPECT_TRUE(watchers_.get(&witness_).contains(&query_));
}

TEST_F(PudWitnessWatchersTest, UnwatchRemovesTheQuery) {
    watchers_.watch(&witness_, &query_);
    watchers_.unwatch(&witness_, &query_);
    EXPECT_TRUE(watchers_.get(&witness_).empty());
}

TEST_F(PudWitnessWatchersTest, UnwatchQueryClearsEveryWitness) {
    watchers_.watch(&witness_, &query_);
    watchers_.watch(&other_, &query_);
    watchers_.unwatch_query(&query_);
    EXPECT_TRUE(watchers_.get(&witness_).empty());
    EXPECT_TRUE(watchers_.get(&other_).empty());
}

TEST_F(PudWitnessWatchersTest, UnknownWitnessHasNoWatchers) {
    EXPECT_TRUE(watchers_.get(&witness_).empty());
}
