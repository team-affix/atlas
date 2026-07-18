// rp_compute_fewer_candidate_goal_value scorer.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/rp_compute_fewer_candidate_goal_value.hpp"
#include "infrastructure/ra_rule_id_set.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetGoalCandidateRuleIds {
    MOCK_METHOD(ra_rule_id_set&, get, (const goal_lineage*));
};

}  // namespace

struct RpComputeFewerCandidateGoalValueTest : public ::testing::Test {
    NiceMock<MockGetGoalCandidateRuleIds> get_ids;
    rp_compute_fewer_candidate_goal_value<MockGetGoalCandidateRuleIds> scorer{get_ids};
    ra_rule_id_set rules;
    goal_lineage gl{nullptr, 0};
};

TEST_F(RpComputeFewerCandidateGoalValueTest, EmptyCandidateSetReturnsZero) {
    EXPECT_CALL(get_ids, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_EQ(scorer.compute_active_goal_value(&gl), 0.0);
}

TEST_F(RpComputeFewerCandidateGoalValueTest, OneCandidateReturnsNegOne) {
    rules.insert(1);
    EXPECT_CALL(get_ids, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_EQ(scorer.compute_active_goal_value(&gl), -1.0);
}

TEST_F(RpComputeFewerCandidateGoalValueTest, ReturnsNegativeCandidateCount) {
    rules.insert(1);
    rules.insert(2);
    rules.insert(3);
    EXPECT_CALL(get_ids, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_EQ(scorer.compute_active_goal_value(&gl), -3.0);
}
