#include <gtest/gtest.h>
#include "infrastructure/dbuct_set_walker.hpp"
#include "infrastructure/lineage_pool.hpp"

struct DbuctSetWalkerTest : public ::testing::Test {
    lineage_pool pool;
    dbuct_set_walker<lineage_pool> walker{pool};

    const goal_lineage* root_goal() {
        return pool.make_goal_lineage(nullptr, subgoal_id{0});
    }

    const resolution_lineage* root_resolution() {
        return pool.make_resolution_lineage(nullptr, rule_id{0});
    }
};

TEST_F(DbuctSetWalkerTest, MakeRootIsEmptySetAndNullGoal) {
    const mcts_node_id root = dbuct_set_walker<lineage_pool>::make_root();
    EXPECT_TRUE(root.first.empty());
    EXPECT_EQ(root.second, nullptr);
}

TEST_F(DbuctSetWalkerTest, GoalChoiceKeepsDecisionSetAndUpdatesGoal) {
    const mcts_node_id root = dbuct_set_walker<lineage_pool>::make_root();
    const goal_lineage* goal = root_goal();
    const mcts_node_id next = walker.walk(root, goal);

    EXPECT_TRUE(next.first.empty());
    EXPECT_EQ(next.second, goal);
}

TEST_F(DbuctSetWalkerTest, RuleChoiceInsertsResolutionAndClearsGoal) {
    const goal_lineage* goal = root_goal();
    const mcts_node_id at_goal{{}, goal};
    const mcts_node_id next = walker.walk(at_goal, rule_id{1});

    ASSERT_EQ(next.first.size(), 1u);
    EXPECT_EQ(next.first.count(pool.make_resolution_lineage(goal, rule_id{1})), 1u);
    EXPECT_EQ(next.second, nullptr);
}

TEST_F(DbuctSetWalkerTest, RuleChoiceCopiesExistingDecisionSet) {
    const resolution_lineage* prior = root_resolution();
    const goal_lineage* goal = root_goal();
    const mcts_node_id at_goal{{prior}, goal};
    const mcts_node_id next = walker.walk(at_goal, rule_id{2});

    ASSERT_EQ(next.first.size(), 2u);
    EXPECT_EQ(next.first.count(prior), 1u);
    EXPECT_EQ(next.first.count(pool.make_resolution_lineage(goal, rule_id{2})), 1u);
    EXPECT_EQ(next.second, nullptr);
}
