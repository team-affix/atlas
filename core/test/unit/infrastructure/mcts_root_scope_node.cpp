#include <gtest/gtest.h>
#include "infrastructure/mcts_root_scope_node.hpp"

TEST(MctsRootScopeNodeTest, RootHasEmptySetAndNullGoal) {
    mcts_root_scope_node root;
    const mcts_scope_node_id node = root.get_mcts_root_node();
    EXPECT_TRUE(node.first.empty());
    EXPECT_EQ(node.second, nullptr);
}
