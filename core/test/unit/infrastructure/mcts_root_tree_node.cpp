#include <gtest/gtest.h>
#include "infrastructure/mcts_root_tree_node.hpp"

TEST(MctsRootTreeNodeTest, RootIsZero) {
    mcts_root_tree_node root;
    EXPECT_EQ(root.get_mcts_root_node(), 0u);
}
