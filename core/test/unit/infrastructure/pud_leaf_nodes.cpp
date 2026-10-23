#include <gtest/gtest.h>
#include "infrastructure/pud_leaf_nodes.hpp"

TEST(PudLeafNodesTest, SetAndCheckLeaf) {
    pud_leaf_nodes leaves;
    leaves.set_leaf(0);
    EXPECT_TRUE(leaves.check_leaf(0));
    EXPECT_FALSE(leaves.check_leaf(1));
}

TEST(PudLeafNodesTest, UnsetLeaf) {
    pud_leaf_nodes leaves;
    leaves.set_leaf(0);
    leaves.unset_leaf(0);
    EXPECT_FALSE(leaves.check_leaf(0));
}
