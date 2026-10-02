#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_leaves.hpp"

struct PudLeavesTest : public ::testing::Test {
    pud_leaves leaves;
    pud_node node{};
    pud_node other{};
};

TEST_F(PudLeavesTest, CheckLeafIsFalseWhenNeverSet) {
    EXPECT_FALSE(leaves.check_leaf(&node));
}

TEST_F(PudLeavesTest, SecondSetLeafThrows) {
    leaves.set_leaf(&node);
    EXPECT_THROW(leaves.set_leaf(&node), std::logic_error);
}

TEST_F(PudLeavesTest, SecondUnsetLeafThrows) {
    leaves.set_leaf(&node);
    leaves.unset_leaf(&node);
    EXPECT_THROW(leaves.unset_leaf(&node), std::logic_error);
}

TEST_F(PudLeavesTest, UnsetLeafThenCheckIsFalse) {
    leaves.set_leaf(&node);
    leaves.unset_leaf(&node);
    EXPECT_FALSE(leaves.check_leaf(&node));
}

TEST_F(PudLeavesTest, UnsettingOneNodeDoesNotUnsetAnother) {
    leaves.set_leaf(&node);
    leaves.set_leaf(&other);
    leaves.unset_leaf(&node);
    EXPECT_TRUE(leaves.check_leaf(&other));
}

TEST_F(PudLeavesTest, SetLeafThenCheckIsTrue) {
    leaves.set_leaf(&node);
    EXPECT_TRUE(leaves.check_leaf(&node));
}
