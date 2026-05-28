// unit_goal_detector recognizes goals with exactly one candidate rule id.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include "infrastructure/unit_goal_detector.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_rule_id_set.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockRuleIdSet : public i_rule_id_set {
    MOCK_METHOD(void, insert, (rule_id), (override));
    MOCK_METHOD(void, erase, (rule_id), (override));
    state_machine<rule_id> iterate() const override { co_return; }
    MOCK_METHOD(size_t, size, (), (const, override));
    MOCK_METHOD(std::unique_ptr<i_rule_id_set>, copy, (), (const, override));
};

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_id_set&, get, (const goal_lineage*), (const, override));
};

struct UnitGoalDetectorTest : public ::testing::Test {
    goal_lineage gl{nullptr, 0};
    MockRuleIdSet rules;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    locator loc;
    unit_goal_detector detector;

    UnitGoalDetectorTest()
        : detector(bind_and_make<unit_goal_detector, i_get_goal_candidate_rule_ids>(
              loc, get_goal_candidate_rule_ids)) {}
};

TEST_F(UnitGoalDetectorTest, NoCandidatesIsNotUnit) {
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(0));
    EXPECT_FALSE(detector.detect(&gl));
}

TEST_F(UnitGoalDetectorTest, ExactlyOneCandidateIsUnit) {
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(1));
    EXPECT_TRUE(detector.detect(&gl));
}

TEST_F(UnitGoalDetectorTest, TwoCandidatesIsNotUnit) {
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(2));
    EXPECT_FALSE(detector.detect(&gl));
}
