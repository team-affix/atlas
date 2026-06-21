// get_unit_resolution picks the front candidate rule for a unit goal and interns
// the corresponding resolution lineage.

#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/get_unit_resolution.hpp"
#include "infrastructure/rule_id_set.hpp"

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::Throw;

struct MockMakeResolutionLineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id));
};

struct MockGetGoalCandidateRuleIds {
    MOCK_METHOD(rule_id_set&, get, (const goal_lineage*));
};

using test_get_unit_resolution_t = get_unit_resolution<MockGetGoalCandidateRuleIds, MockMakeResolutionLineage>;

struct GetUnitResolutionTest : public ::testing::Test {
    MockMakeResolutionLineage make_resolution_lineage;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    test_get_unit_resolution_t sut{get_goal_candidate_rule_ids, make_resolution_lineage};

    goal_lineage gl{nullptr, 0};
    rule_id_set candidates;
};

TEST_F(GetUnitResolutionTest, SingleCandidateReturnsResolution) {
    static constexpr rule_id kCandidate = 5;
    candidates.insert(kCandidate);
    resolution_lineage unit_rl{&gl, kCandidate};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution_lineage, make_resolution_lineage(&gl, kCandidate))
        .WillOnce(Return(&unit_rl));

    EXPECT_EQ(sut.get(&gl), &unit_rl);
}

TEST_F(GetUnitResolutionTest, ZeroCandidatesThrows) {
    rule_id_set empty_candidates;
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(empty_candidates));
    EXPECT_THROW(sut.get(&gl), std::logic_error);
}

TEST_F(GetUnitResolutionTest, UnknownGoalThrows) {
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl))
        .WillOnce(Throw(std::out_of_range{"unknown goal"}));
    EXPECT_THROW(sut.get(&gl), std::out_of_range);
}
