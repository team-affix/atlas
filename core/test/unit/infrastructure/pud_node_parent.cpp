// pud_node_parent: store and get by child id.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/pud_node_parent.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudNodeParentTest : public ::testing::Test {
    PudNodeParentTest()
        : axiom_{pud_rule_id::axiom{0}}
        , child_{pud_rule_id::inference{&axiom_, 0, &axiom_}}
        , parents_() {}

    pud_rule_id axiom_;
    pud_rule_id child_;
    pud_node_parent parents_;
};

TEST_F(PudNodeParentTest, StoreThenGet) {
    parents_.store(&child_, &axiom_);
    EXPECT_EQ(parents_.get(&child_), &axiom_);
}

TEST_F(PudNodeParentTest, RootStoresNullptr) {
    parents_.store(&axiom_, nullptr);
    EXPECT_EQ(parents_.get(&axiom_), nullptr);
}

TEST_F(PudNodeParentTest, GetUnknownThrows) {
    EXPECT_THROW(parents_.get(&axiom_), std::out_of_range);
}
