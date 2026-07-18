// rp_heuristic_rollout: dispatches goal vs rule choice lists to choosers.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstddef>
#include <vector>
#include "infrastructure/rp_heuristic_rollout.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/rule.hpp"

using ::testing::ElementsAre;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockRolloutChooseGoal {
    MOCK_METHOD(const goal_lineage*, rollout_choose_goal,
                (const std::vector<const goal_lineage*>&));
};

struct MockRolloutChooseRule {
    MOCK_METHOD(rule_id, rollout_choose_rule, (const std::vector<rule_id>&));
};

struct ChoiceList {
    std::vector<mcts_choice> choices;

    size_t size() const { return choices.size(); }
    mcts_choice at(size_t i) const { return choices[i]; }
};

using rollout_t = rp_heuristic_rollout<MockRolloutChooseGoal, MockRolloutChooseRule>;

}  // namespace

struct RpHeuristicRolloutTest : public ::testing::Test {
    StrictMock<MockRolloutChooseGoal> goal;
    StrictMock<MockRolloutChooseRule> rule;
    rollout_t rollout{goal, rule};

    goal_lineage g0{nullptr, 0};
    goal_lineage g1{nullptr, 1};
};

TEST_F(RpHeuristicRolloutTest, DispatchesGoalListToGoalChooser) {
    ChoiceList list;
    list.choices = {&g0, &g1};
    EXPECT_CALL(goal, rollout_choose_goal(ElementsAre(&g0, &g1)))
        .WillOnce(Return(&g1));
    EXPECT_CALL(rule, rollout_choose_rule).Times(0);
    const mcts_choice result = rollout.rollout_choose(list, list);
    EXPECT_EQ(std::get<const goal_lineage*>(result), &g1);
}

TEST_F(RpHeuristicRolloutTest, DispatchesRuleListToRuleChooser) {
    ChoiceList list;
    list.choices = {rule_id{3}, rule_id{7}};
    EXPECT_CALL(goal, rollout_choose_goal).Times(0);
    EXPECT_CALL(rule, rollout_choose_rule(ElementsAre(rule_id{3}, rule_id{7})))
        .WillOnce(Return(rule_id{7}));
    const mcts_choice result = rollout.rollout_choose(list, list);
    EXPECT_EQ(std::get<rule_id>(result), rule_id{7});
}

TEST_F(RpHeuristicRolloutTest, SingleGoalStillUsesGoalBranch) {
    ChoiceList list;
    list.choices = {&g0};
    EXPECT_CALL(goal, rollout_choose_goal(ElementsAre(&g0))).WillOnce(Return(&g0));
    EXPECT_CALL(rule, rollout_choose_rule).Times(0);
    const mcts_choice result = rollout.rollout_choose(list, list);
    EXPECT_EQ(std::get<const goal_lineage*>(result), &g0);
}
