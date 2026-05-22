#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/unit_goal_detector.hpp"
#include "../../../core/hpp/interfaces/i_unit_goal_detector.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_candidate_rules.hpp"
#include "../../../core/hpp/interfaces/i_rule_set.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockRuleSet : public i_rule_set {
    MOCK_METHOD(void, insert, (const rule*), (override));
    MOCK_METHOD(void, erase, (const rule*), (override));
    MOCK_METHOD(void, accept, (i_visitor<const rule*>&), (const, override));
    MOCK_METHOD(size_t, size, (), (const, override));
};

struct MockGetGoalCandidateRules : public i_get_goal_candidate_rules {
    MOCK_METHOD(i_rule_set&, get, (const goal_lineage*), (override));
};

struct UnitGoalDetectorTest : public ::testing::Test {
    goal_lineage gl{nullptr, nullptr};
    MockRuleSet rules;
    MockGetGoalCandidateRules ggcr;
    unit_goal_detector detector{ggcr};
    i_unit_goal_detector& sut{detector};
};

TEST_F(UnitGoalDetectorTest, NoCandidatesIsNotUnit) {
    EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(0));
    EXPECT_FALSE(sut.detect(&gl));
}

TEST_F(UnitGoalDetectorTest, ExactlyOneCandidateIsUnit) {
    EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(1));
    EXPECT_TRUE(sut.detect(&gl));
}

TEST_F(UnitGoalDetectorTest, TwoCandidatesIsNotUnit) {
    EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(2));
    EXPECT_FALSE(sut.detect(&gl));
}
