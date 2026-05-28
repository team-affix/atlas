// get_unit_resolution picks the sole candidate rule for a unit goal and interns
// the corresponding resolution lineage.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/get_unit_resolution.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id), (override));
};

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_id_set&, get, (const goal_lineage*), (const, override));
};

struct GetUnitResolutionTest : public ::testing::Test {
    MockMakeResolutionLineage make_resolution_lineage;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    get_unit_resolution sut{get_goal_candidate_rule_ids, make_resolution_lineage};

    goal_lineage gl{nullptr, 0};
    rule_id_set candidates;
};

TEST_F(GetUnitResolutionTest, ReturnsFirstCandidateResolution) {
    static constexpr rule_id kCandidate = 5;
    candidates.insert(kCandidate);
    resolution_lineage unit_rl{&gl, kCandidate};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution_lineage, make_resolution_lineage(&gl, kCandidate))
        .WillOnce(Return(&unit_rl));

    EXPECT_EQ(sut.get(&gl), &unit_rl);
}
