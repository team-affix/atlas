// compute_fewer_candidate_goal_value scorer.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/compute_fewer_candidate_goal_value.hpp"
#include "infrastructure/ra_rule_id_set.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetGoalCandidateRuleIds {
    MOCK_METHOD(ra_rule_id_set&, get, (const goal_lineage*));
};

}  // namespace

struct ComputeFewerCandidateGoalValueTest : public ::testing::Test {
    NiceMock<MockGetGoalCandidateRuleIds> get_ids;
    compute_fewer_candidate_goal_value<MockGetGoalCandidateRuleIds> scorer{get_ids};
    ra_rule_id_set rules;
    goal_lineage gl{nullptr, 0};
};

TEST_F(ComputeFewerCandidateGoalValueTest, ReturnsNegativeCandidateCount) {
    rules.insert(1);
    rules.insert(2);
    rules.insert(3);
    EXPECT_CALL(get_ids, get(&gl)).WillOnce(ReturnRef(rules));
    EXPECT_EQ(scorer.compute_active_goal_value(&gl), -3.0);
}
