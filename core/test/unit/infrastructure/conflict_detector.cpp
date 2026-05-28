// conflict_detector flags goals with zero applicable candidate rules. Tests mock
// i_get_goal_candidate_rules and i_rule_set::size to assert conflict iff size is zero.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/conflict_detector.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_candidate_rules.hpp"
#include "../../../core/hpp/interfaces/i_rule_set.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

struct MockRuleSet : public i_rule_set {
    MOCK_METHOD(void, insert, (rule_id), (override));
    MOCK_METHOD(void, erase, (rule_id), (override));
    state_machine<rule_id> iterate() const override { co_return; }
    MOCK_METHOD(size_t, size, (), (const, override));
};

struct MockGetGoalCandidateRules : public i_get_goal_candidate_rules {
    MOCK_METHOD(i_rule_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_set&, get, (const goal_lineage*), (const, override));
};

struct ConflictDetectorTest : public ::testing::Test {
    goal_lineage gl{nullptr, nullptr};
    expr head{expr::var{0}};
    rule r{&head, {}};
    MockRuleSet rules;
    MockGetGoalCandidateRules ggcr;
    conflict_detector detector{ggcr};
};

TEST_F(ConflictDetectorTest, NoCandidatesIsConflict) {
    EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(0));
    EXPECT_TRUE(detector.detect(&gl));
}

TEST_F(ConflictDetectorTest, OneCandidateIsNotConflict) {
    EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(1));
    EXPECT_FALSE(detector.detect(&gl));
}

TEST_F(ConflictDetectorTest, ManyCandidatesIsNotConflict) {
    EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(2));
    EXPECT_FALSE(detector.detect(&gl));
}
