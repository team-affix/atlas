// unit_goal_detector recognizes goals with exactly one candidate rule id.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/unit_goal_detector.hpp"
#include "infrastructure/ra_rule_id_set.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockGetGoalCandidateRuleIds {
    MOCK_METHOD(ra_rule_id_set&, get_mutable, (const goal_lineage*));
    ra_rule_id_set& get(const goal_lineage* gl) { return get_mutable(gl); }
};

using TestUnitGoalDetector = unit_goal_detector<MockGetGoalCandidateRuleIds>;

struct UnitGoalDetectorTest : public ::testing::Test {
    goal_lineage gl{nullptr, 0};
    ra_rule_id_set rules;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    TestUnitGoalDetector detector{get_goal_candidate_rule_ids};
};

TEST_F(UnitGoalDetectorTest, NoCandidatesIsNotUnit) {
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_FALSE(detector.detect(&gl));
}

TEST_F(UnitGoalDetectorTest, ExactlyOneCandidateIsUnit) {
    rules.insert(0);
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_TRUE(detector.detect(&gl));
}

TEST_F(UnitGoalDetectorTest, TwoCandidatesIsNotUnit) {
    rules.insert(0);
    rules.insert(1);
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_FALSE(detector.detect(&gl));
}
