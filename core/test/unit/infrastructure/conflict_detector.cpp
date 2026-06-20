// conflict_detector flags goals with zero applicable candidate rule ids.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/conflict_detector.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockRuleIdSet {
    MOCK_METHOD(size_t, size, (), (const));
};

struct MockGetGoalCandidateRuleIds {
    MOCK_METHOD(MockRuleIdSet&, get, (const goal_lineage*));
};

using TestConflictDetector = conflict_detector<MockGetGoalCandidateRuleIds>;

struct ConflictDetectorTest : public ::testing::Test {
    goal_lineage gl{nullptr, 0};
    MockRuleIdSet rules;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    TestConflictDetector detector{get_goal_candidate_rule_ids};
};

TEST_F(ConflictDetectorTest, NoCandidatesIsConflict) {
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(0));
    EXPECT_TRUE(detector.detect(&gl));
}

TEST_F(ConflictDetectorTest, OneCandidateIsNotConflict) {
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(1));
    EXPECT_FALSE(detector.detect(&gl));
}

TEST_F(ConflictDetectorTest, ManyCandidatesIsNotConflict) {
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_CALL(rules, size()).WillOnce(Return(2));
    EXPECT_FALSE(detector.detect(&gl));
}
