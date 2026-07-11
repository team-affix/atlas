#include <gtest/gtest.h>
#include "infrastructure/dbuct_tree_walker.hpp"
#include "infrastructure/lineage_pool.hpp"

struct DbuctTreeWalkerTest : public ::testing::Test {
    lineage_pool pool;
    dbuct_tree_walker walker;

    const goal_lineage* root_goal() {
        return pool.make_goal_lineage(nullptr, subgoal_id{0});
    }
};

TEST_F(DbuctTreeWalkerTest, FirstWalkAllocatesNextId) {
    const size_t root = 0;
    const size_t child = walker.walk(root, root_goal());
    EXPECT_EQ(child, 1u);
}

TEST_F(DbuctTreeWalkerTest, SameParentAndChoiceReuseChildId) {
    const size_t root = 0;
    const goal_lineage* goal = root_goal();
    const size_t first = walker.walk(root, goal);
    const size_t second = walker.walk(root, goal);
    EXPECT_EQ(first, second);
}

TEST_F(DbuctTreeWalkerTest, DifferentChoicesFromSameParentGetDistinctIds) {
    const size_t root = 0;
    const goal_lineage* goal0 = pool.make_goal_lineage(nullptr, subgoal_id{0});
    const goal_lineage* goal1 = pool.make_goal_lineage(nullptr, subgoal_id{1});
    const size_t child0 = walker.walk(root, goal0);
    const size_t child1 = walker.walk(root, goal1);
    EXPECT_NE(child0, child1);
}

TEST_F(DbuctTreeWalkerTest, RuleChoiceAllocatesFreshTreeNode) {
    const size_t root = 0;
    const size_t child = walker.walk(root, rule_id{0});
    EXPECT_EQ(child, 1u);
    EXPECT_EQ(walker.walk(root, rule_id{0}), child);
}
