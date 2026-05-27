// unit_goal_detector recognizes goals with exactly one candidate rule. Tests mock rule
// set size lookup and assert detect() is true only when size equals one.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/unit_goal_detector.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_candidate_rules.hpp"
#include "../../../core/hpp/interfaces/i_rule_set.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockRuleSet : public i_rule_set {
    MOCK_METHOD(void, insert, (const rule*), (override));
    MOCK_METHOD(void, erase, (const rule*), (override));
    state_machine<const rule*> iterate() const override { co_return; }
    MOCK_METHOD(size_t, size, (), (const, override));
};

struct MockGetGoalCandidateRules : public i_get_goal_candidate_rules {
    MOCK_METHOD(i_rule_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_set&, get, (const goal_lineage*), (const, override));
};

struct UnitGoalDetectorTest : public ::testing::Test {
    goal_lineage gl{nullptr, nullptr};
    MockRuleSet rules;
    MockGetGoalCandidateRules ggcr;
    unit_goal_detector detector{ggcr};
};

TEST_F(UnitGoalDetectorTest, NoCandidatesIsNotUnit) {
    EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(0));
    EXPECT_FALSE(detector.detect(&gl));
}

TEST_F(UnitGoalDetectorTest, ExactlyOneCandidateIsUnit) {
    EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(1));
    EXPECT_TRUE(detector.detect(&gl));
}

TEST_F(UnitGoalDetectorTest, TwoCandidatesIsNotUnit) {
    EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(2));
    EXPECT_FALSE(detector.detect(&gl));
}
