#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_leaf_nodes.hpp"

struct PudLeafNodesTest : public ::testing::Test {
    pud_leaf_nodes leaves;
    pud_node_id node = 1;
    pud_node_id other = 2;
};

TEST_F(PudLeafNodesTest, CheckLeafIsFalseWhenNeverSet) {
    EXPECT_FALSE(leaves.check_leaf(node));
}

TEST_F(PudLeafNodesTest, SecondSetLeafThrows) {
    leaves.set_leaf(node);
    EXPECT_THROW(leaves.set_leaf(node), std::logic_error);
}

TEST_F(PudLeafNodesTest, SecondUnsetLeafThrows) {
    leaves.set_leaf(node);
    leaves.unset_leaf(node);
    EXPECT_THROW(leaves.unset_leaf(node), std::logic_error);
}

TEST_F(PudLeafNodesTest, UnsetLeafThenCheckIsFalse) {
    leaves.set_leaf(node);
    leaves.unset_leaf(node);
    EXPECT_FALSE(leaves.check_leaf(node));
}

TEST_F(PudLeafNodesTest, UnsettingOneNodeDoesNotUnsetAnother) {
    leaves.set_leaf(node);
    leaves.set_leaf(other);
    leaves.unset_leaf(node);
    EXPECT_TRUE(leaves.check_leaf(other));
}

TEST_F(PudLeafNodesTest, SetLeafThenCheckIsTrue) {
    leaves.set_leaf(node);
    EXPECT_TRUE(leaves.check_leaf(node));
}
