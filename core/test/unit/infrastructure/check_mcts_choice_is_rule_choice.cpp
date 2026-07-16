#include <gtest/gtest.h>
#include "infrastructure/check_mcts_choice_is_rule_choice.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"

TEST(CheckMctsChoiceIsRuleChoiceTest, RuleIdIsRuleChoice) {
    check_mcts_choice_is_rule_choice checker;
    mcts_choice choice = rule_id{7};
    EXPECT_TRUE(checker.check_is_rule_choice(choice));
}

TEST(CheckMctsChoiceIsRuleChoiceTest, GoalLineageIsNotRuleChoice) {
    check_mcts_choice_is_rule_choice checker;
    goal_lineage gl{nullptr, 0};
    mcts_choice choice = &gl;
    EXPECT_FALSE(checker.check_is_rule_choice(choice));
}
