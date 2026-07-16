// mcts_choice_hash: hashes mcts_choice variants for unordered containers.

#include <gtest/gtest.h>
#include <unordered_map>
#include <unordered_set>
#include "value_objects/mcts_choice_hash.hpp"

struct MctsChoiceHashTest : public ::testing::Test {
    mcts_choice_hash hasher;

    goal_lineage goal0{nullptr, 0};
    goal_lineage goal1{nullptr, 1};
};

TEST_F(MctsChoiceHashTest, SameGoalPointerHashesEqual) {
    const mcts_choice choice0{&goal0};
    const mcts_choice choice0_copy{&goal0};
    EXPECT_EQ(hasher(choice0), hasher(choice0_copy));
}

TEST_F(MctsChoiceHashTest, DifferentGoalPointerHashesDiffer) {
    const mcts_choice choice0{&goal0};
    const mcts_choice choice1{&goal1};
    EXPECT_NE(hasher(choice0), hasher(choice1));
}

TEST_F(MctsChoiceHashTest, SameRuleIdHashesEqual) {
    const mcts_choice choice0{rule_id{3}};
    const mcts_choice choice0_copy{rule_id{3}};
    EXPECT_EQ(hasher(choice0), hasher(choice0_copy));
}

TEST_F(MctsChoiceHashTest, DifferentRuleIdHashesDiffer) {
    const mcts_choice choice0{rule_id{0}};
    const mcts_choice choice1{rule_id{1}};
    EXPECT_NE(hasher(choice0), hasher(choice1));
}

TEST_F(MctsChoiceHashTest, GoalAndRuleAlternativesTaggedDistinctly) {
    const mcts_choice goal_choice{&goal0};
    const mcts_choice rule_choice{rule_id{0}};
    EXPECT_NE(hasher(goal_choice), hasher(rule_choice));
}

TEST_F(MctsChoiceHashTest, WorksAsUnorderedSetKey) {
    const mcts_choice goal_choice{&goal0};
    const mcts_choice rule_choice{rule_id{0}};
    std::unordered_set<mcts_choice, mcts_choice_hash> keys;

    keys.insert(goal_choice);
    keys.insert(rule_choice);

    EXPECT_EQ(keys.size(), 2u);
    EXPECT_TRUE(keys.contains(goal_choice));
    EXPECT_TRUE(keys.contains(rule_choice));
}

TEST_F(MctsChoiceHashTest, WorksAsUnorderedMapKey) {
    const mcts_choice goal_choice{&goal0};
    const mcts_choice rule_choice{rule_id{0}};
    std::unordered_map<mcts_choice, int, mcts_choice_hash> counts;

    counts[goal_choice] = 1;
    counts[rule_choice] = 2;

    EXPECT_EQ(counts.at(goal_choice), 1);
    EXPECT_EQ(counts.at(rule_choice), 2);
    EXPECT_EQ(counts.size(), 2u);
}
