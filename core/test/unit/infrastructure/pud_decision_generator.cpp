#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_decision_generator.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {
struct MockChooseUnfoldLeafRule {
    MOCK_METHOD(pud_node_id, choose_rule, (const std::set<pud_node_id>&));
};

struct MockChooseUnfoldSubgoal {
    MOCK_METHOD(pud_body_goal_id, choose_subgoal, (pud_node_id));
};

struct PudDecisionGeneratorTest : public ::testing::Test {
    StrictMock<MockChooseUnfoldLeafRule> rules;
    StrictMock<MockChooseUnfoldSubgoal> subgoals;
    pud_decision_generator<StrictMock<MockChooseUnfoldLeafRule>, StrictMock<MockChooseUnfoldSubgoal>>
        generator{rules, subgoals};
};

TEST_F(PudDecisionGeneratorTest, DecisionPairsTheChosenRuleWithItsChosenSubgoal) {
    EXPECT_CALL(rules, choose_rule(std::set<pud_node_id>({3, 8}))).WillOnce(Return(8));
    EXPECT_CALL(subgoals, choose_subgoal(8u)).WillOnce(Return(2));
    EXPECT_EQ(generator.generate({3, 8}), (pud_decision{8, 2}));
}

TEST_F(PudDecisionGeneratorTest, SubgoalIsChosenForTheChosenRuleOnly) {
    InSequence in_order;
    EXPECT_CALL(rules, choose_rule(_)).WillOnce(Return(5));
    EXPECT_CALL(subgoals, choose_subgoal(5u)).WillOnce(Return(0));
    generator.generate({5, 6, 7});
}

TEST_F(PudDecisionGeneratorTest, EveryOpenLeafIsOfferedToTheRuleChooser) {
    EXPECT_CALL(rules, choose_rule(std::set<pud_node_id>({1, 4, 9}))).WillOnce(Return(1));
    EXPECT_CALL(subgoals, choose_subgoal(1u)).WillOnce(Return(0));
    generator.generate({9, 1, 4});
}

}
