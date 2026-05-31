// get_unit_resolution picks the sole candidate rule for a unit goal and interns
// the corresponding resolution lineage.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/get_unit_resolution.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_rule_id_set.hpp"

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

TEST_F(GetUnitResolutionTest, ReturnsFirstCandidateResolution) {
    static constexpr rule_id kCandidate = 5;
    candidates.insert(kCandidate);
    resolution_lineage unit_rl{&gl, kCandidate};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution_lineage, make_resolution_lineage(&gl, kCandidate))
        .WillOnce(Return(&unit_rl));

    EXPECT_EQ(sut.get(&gl), &unit_rl);
}

TEST_F(GetUnitResolutionTest, EmptyCandidatesReturnsNullptr) {
    rule_id_set empty_candidates;
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(empty_candidates));
    EXPECT_EQ(sut.get(&gl), nullptr);
}

namespace {

struct mock_rule_id_set : i_rule_id_set {
    coroutine<rule_id, void> iterate() const override {
        static constexpr rule_id kFirst = 5;
        static constexpr rule_id kSecond = 7;
        co_yield kFirst;
        co_yield kSecond;
    }
    void insert(rule_id) override {}
    void erase(rule_id) override {}
    size_t size() const override { return 2; }
    std::unique_ptr<i_rule_id_set> copy() const override {
        return std::make_unique<mock_rule_id_set>();
    }
};

} // namespace

TEST_F(GetUnitResolutionTest, MultipleCandidatesReturnsFirstOnly) {
    static constexpr rule_id kFirst = 5;
    mock_rule_id_set ordered_candidates;
    resolution_lineage unit_rl{&gl, kFirst};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(ordered_candidates));
    EXPECT_CALL(make_resolution_lineage, make_resolution_lineage(&gl, kFirst))
        .WillOnce(Return(&unit_rl));

    EXPECT_EQ(sut.get(&gl), &unit_rl);
}
