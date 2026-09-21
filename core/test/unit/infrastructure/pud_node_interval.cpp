// pud_node_interval: store overwrites; get throws on miss.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/pud_node_interval.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudNodeIntervalTest : public ::testing::Test {
    PudNodeIntervalTest()
        : open_a_(10)
        , close_a_(40)
        , open_b_(15)
        , close_b_(20)
        , interval_a_{om_label(&open_a_), om_label(&close_a_)}
        , interval_b_{om_label(&open_b_), om_label(&close_b_)}
        , axiom_{pud_rule_id::axiom{0}}
        , intervals_() {}

    uint64_t open_a_;
    uint64_t close_a_;
    uint64_t open_b_;
    uint64_t close_b_;
    om_interval interval_a_;
    om_interval interval_b_;
    pud_rule_id axiom_;
    pud_node_interval intervals_;
};

TEST_F(PudNodeIntervalTest, StoreThenGet) {
    intervals_.store(&axiom_, interval_a_);
    EXPECT_EQ(intervals_.get(&axiom_).open.rank_ptr(), interval_a_.open.rank_ptr());
    EXPECT_EQ(intervals_.get(&axiom_).close.rank_ptr(), interval_a_.close.rank_ptr());
}

TEST_F(PudNodeIntervalTest, GetUnknownThrows) {
    EXPECT_THROW(intervals_.get(&axiom_), std::out_of_range);
}

TEST_F(PudNodeIntervalTest, ContainsIsFalseUntilStore) {
    EXPECT_FALSE(intervals_.contains(&axiom_));
    intervals_.store(&axiom_, interval_a_);
    EXPECT_TRUE(intervals_.contains(&axiom_));
}

TEST_F(PudNodeIntervalTest, StoreOverwrites) {
    intervals_.store(&axiom_, interval_a_);
    intervals_.store(&axiom_, interval_b_);
    EXPECT_EQ(intervals_.get(&axiom_).open.rank_ptr(), interval_b_.open.rank_ptr());
    EXPECT_EQ(intervals_.get(&axiom_).close.rank_ptr(), interval_b_.close.rank_ptr());
}
