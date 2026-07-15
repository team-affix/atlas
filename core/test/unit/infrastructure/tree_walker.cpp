#include <gtest/gtest.h>
#include "infrastructure/tree_walker.hpp"
#include "infrastructure/lineage_pool.hpp"

struct TreeWalkerTest : public ::testing::Test {
    lineage_pool pool;
    tree_walker walker;

    const goal_lineage* root_goal() {
        return pool.make_goal_lineage(nullptr, subgoal_id{0});
    }
};

TEST_F(TreeWalkerTest, FirstWalkAllocatesNextId) {
    const mcts_tree_node_id root = 0;
    const mcts_tree_node_id child = walker.walk(root, root_goal());
    EXPECT_EQ(child, 1u);
}

TEST_F(TreeWalkerTest, SameParentAndChoiceReuseChildId) {
    const mcts_tree_node_id root = 0;
    const goal_lineage* goal = root_goal();
    const mcts_tree_node_id first = walker.walk(root, goal);
    const mcts_tree_node_id second = walker.walk(root, goal);
    EXPECT_EQ(first, second);
}

TEST_F(TreeWalkerTest, DifferentChoicesFromSameParentGetDistinctIds) {
    const mcts_tree_node_id root = 0;
    const goal_lineage* goal0 = pool.make_goal_lineage(nullptr, subgoal_id{0});
    const goal_lineage* goal1 = pool.make_goal_lineage(nullptr, subgoal_id{1});
    const mcts_tree_node_id child0 = walker.walk(root, goal0);
    const mcts_tree_node_id child1 = walker.walk(root, goal1);
    EXPECT_NE(child0, child1);
}

TEST_F(TreeWalkerTest, RuleChoiceAllocatesFreshTreeNode) {
    const mcts_tree_node_id root = 0;
    const mcts_tree_node_id child = walker.walk(root, rule_id{0});
    EXPECT_EQ(child, 1u);
    EXPECT_EQ(walker.walk(root, rule_id{0}), child);
}
