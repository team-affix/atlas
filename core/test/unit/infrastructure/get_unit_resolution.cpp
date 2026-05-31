// get_unit_resolution picks the front candidate rule for a unit goal and interns
// the corresponding resolution lineage.

#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/get_unit_resolution.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::Throw;

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
    locator loc;
    get_unit_resolution sut;

    GetUnitResolutionTest() : sut(init_sut()) {}

    get_unit_resolution init_sut() {
        loc.bind_as<i_get_goal_candidate_rule_ids>(get_goal_candidate_rule_ids);
        loc.bind_as<i_make_resolution_lineage>(make_resolution_lineage);
        return get_unit_resolution{loc};
    }

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
