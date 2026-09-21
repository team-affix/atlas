// pud_node_base_interval: store and get by id.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/pud_node_base_interval.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudNodeBaseIntervalTest : public ::testing::Test {
    PudNodeBaseIntervalTest()
        : open_(10)
        , close_(40)
        , interval_{om_label(&open_), om_label(&close_)}
        , axiom_{pud_rule_id::axiom{0}}
        , intervals_() {}

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    pud_rule_id axiom_;
    pud_node_base_interval intervals_;
};

TEST_F(PudNodeBaseIntervalTest, StoreThenGet) {
    intervals_.store(&axiom_, interval_);
    EXPECT_EQ(intervals_.get(&axiom_).open.rank_ptr(), interval_.open.rank_ptr());
    EXPECT_EQ(intervals_.get(&axiom_).close.rank_ptr(), interval_.close.rank_ptr());
}

TEST_F(PudNodeBaseIntervalTest, GetUnknownThrows) {
    EXPECT_THROW(intervals_.get(&axiom_), std::out_of_range);
}
