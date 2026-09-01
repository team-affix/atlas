// phoenix_policy_choose: dispatches goal vs rule lists to policy choosers.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstddef>
#include <vector>
#include "infrastructure/phoenix_policy_choose.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct ChoiceList {
    std::vector<mcts_choice> choices;

    size_t size() const { return choices.size(); }
    mcts_choice at(size_t i) const { return choices[i]; }
};

struct MockCheckIsRuleChoice {
    MOCK_METHOD(bool, check_is_rule_choice, (const mcts_choice&), (const));
};

struct MockPolicyChooseGoal {
    MOCK_METHOD(mcts_choice, policy_choose,
                (const size_t&, const ChoiceList&, const ChoiceList&), ());
};

struct MockPolicyChooseRule {
    MOCK_METHOD(mcts_choice, policy_choose,
                (const size_t&, const ChoiceList&, const ChoiceList&), ());
};

using test_phoenix_policy_choose_t = phoenix_policy_choose<
    MockCheckIsRuleChoice, MockPolicyChooseGoal, MockPolicyChooseRule>;

} // namespace

struct PhoenixPolicyChooseTest : public ::testing::Test {
    StrictMock<MockCheckIsRuleChoice> check;
    StrictMock<MockPolicyChooseGoal> goal;
    StrictMock<MockPolicyChooseRule> rule;
    test_phoenix_policy_choose_t choose{check, goal, rule};

    goal_lineage g0{nullptr, 0};
    goal_lineage g1{nullptr, 1};
    const size_t node = 7;
};

TEST_F(PhoenixPolicyChooseTest, DispatchesGoalListToGoalChooser) {
    ChoiceList list;
    list.choices = {&g0, &g1};
    EXPECT_CALL(check, check_is_rule_choice(_)).WillOnce(Return(false));
    EXPECT_CALL(goal, policy_choose(node, _, _))
        .WillOnce(Return(mcts_choice{&g1}));
    EXPECT_CALL(rule, policy_choose).Times(0);
    const mcts_choice result = choose.policy_choose(node, list, list);
    EXPECT_EQ(std::get<const goal_lineage*>(result), &g1);
}

TEST_F(PhoenixPolicyChooseTest, DispatchesRuleListToRuleChooser) {
    ChoiceList list;
    list.choices = {rule_id{3}, rule_id{7}};
    EXPECT_CALL(check, check_is_rule_choice(_)).WillOnce(Return(true));
    EXPECT_CALL(goal, policy_choose).Times(0);
    EXPECT_CALL(rule, policy_choose(node, _, _))
        .WillOnce(Return(mcts_choice{rule_id{7}}));
    const mcts_choice result = choose.policy_choose(node, list, list);
    EXPECT_EQ(std::get<rule_id>(result), 7u);
}

TEST_F(PhoenixPolicyChooseTest, SingleGoalStillUsesGoalBranch) {
    ChoiceList list;
    list.choices = {&g0};
    EXPECT_CALL(check, check_is_rule_choice(_)).WillOnce(Return(false));
    EXPECT_CALL(goal, policy_choose(node, _, _))
        .WillOnce(Return(mcts_choice{&g0}));
    EXPECT_CALL(rule, policy_choose).Times(0);
    const mcts_choice result = choose.policy_choose(node, list, list);
    EXPECT_EQ(std::get<const goal_lineage*>(result), &g0);
}
