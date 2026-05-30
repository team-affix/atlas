// Random decision generator: picks a uniform random active goal and candidate rule,
// then interns resolution via make_resolution_lineage. Tests stub that call by
// constructing resolution_lineage{goal, rule}. A fixture set stores them so returned
// pointers stay valid; assertions only check against the configured possibilities.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <random>
#include <set>
#include <vector>
#include "locator_fixture.hpp"
#include "infrastructure/random_decision_generator.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_iterate_active_goals.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"

using ::testing::_;
using ::testing::ReturnRef;

namespace {

constexpr size_t kLargeGoalCount = 50;
constexpr size_t kLargeCandidateCount = 100;
constexpr size_t kCoverageTrials = 500;

coroutine<const goal_lineage*, void> single_goal(const goal_lineage* gl) {
    co_yield gl;
}

coroutine<const goal_lineage*, void> many_goals(
    const std::vector<const goal_lineage*>& goals) {
    for (const goal_lineage* gl : goals)
        co_yield gl;
}

std::vector<goal_lineage> make_goals(size_t count) {
    std::vector<goal_lineage> goals;
    goals.reserve(count);
    for (size_t i = 0; i < count; ++i)
        goals.emplace_back(nullptr, i);
    return goals;
}

std::vector<const goal_lineage*> goal_ptrs(const std::vector<goal_lineage>& goals) {
    std::vector<const goal_lineage*> ptrs;
    ptrs.reserve(goals.size());
    for (const goal_lineage& gl : goals)
        ptrs.push_back(&gl);
    return ptrs;
}

void fill_candidates(rule_id_set& candidates, size_t count) {
    for (rule_id r = 0; r < count; ++r)
        candidates.insert(r);
}

auto stub_make_resolution_lineage(std::set<resolution_lineage>& resolutions) {
    return [&resolutions](const goal_lineage* goal, rule_id rid) {
        auto [it, _] = resolutions.emplace(goal, rid);
        return &*it;
    };
}

}  // namespace

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id), (override));
};

struct MockIterateActiveGoals : public i_iterate_active_goals {
    MOCK_METHOD((coroutine<const goal_lineage*, void>), iterate_active_goals, (), (const, override));
};

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get_mutable, (const goal_lineage*), ());
    MOCK_METHOD(const i_rule_id_set&, get_const, (const goal_lineage*), (const));
    i_rule_id_set& get(const goal_lineage* gl) override { return get_mutable(gl); }
    const i_rule_id_set& get(const goal_lineage* gl) const override { return get_const(gl); }
};

struct RandomDecisionGeneratorTest : public ::testing::Test {
    locator loc;
    MockMakeResolutionLineage lp;
    MockIterateActiveGoals iterate_active_goals;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    std::mt19937 rng{0};
    random_decision_generator generator;

    RandomDecisionGeneratorTest() : generator(init_generator()) {}

    random_decision_generator init_generator() {
        loc.bind_as<i_make_resolution_lineage>(lp);
        loc.bind_as<i_iterate_active_goals>(iterate_active_goals);
        loc.bind_as<i_get_goal_candidate_rule_ids>(get_goal_candidate_rule_ids);
        return random_decision_generator{loc, rng};
    }

    std::set<resolution_lineage> resolutions;
    goal_lineage gl{nullptr, 0};
    rule_id_set candidates;
};

TEST_F(RandomDecisionGeneratorTest, GenerateResolvesChosenGoalAndRule) {
    static constexpr rule_id kRule = 0;
    candidates.insert(kRule);
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .WillOnce([&] { return single_goal(&gl); });
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable(&gl)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(lp, make_resolution_lineage(&gl, kRule))
        .WillOnce(stub_make_resolution_lineage(resolutions));
    const resolution_lineage* result = generator.generate();
    EXPECT_EQ(*result, (resolution_lineage{&gl, kRule}));
}

TEST_F(RandomDecisionGeneratorTest, GeneratePicksAmongManyCandidates) {
    fill_candidates(candidates, kLargeCandidateCount);
    std::set<rule_id> chosen;
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .Times(kCoverageTrials)
        .WillRepeatedly([&] { return single_goal(&gl); });
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable(&gl))
        .Times(kCoverageTrials)
        .WillRepeatedly(ReturnRef(candidates));
    EXPECT_CALL(lp, make_resolution_lineage(&gl, _))
        .Times(kCoverageTrials)
        .WillRepeatedly(stub_make_resolution_lineage(resolutions));
    for (size_t i = 0; i < kCoverageTrials; ++i) {
        const resolution_lineage* result = generator.generate();
        EXPECT_EQ(result->parent, &gl);
        EXPECT_LT(result->idx, kLargeCandidateCount);
        chosen.insert(result->idx);
    }
    EXPECT_GE(chosen.size(), kLargeCandidateCount / 2)
        << "expected broad coverage across large candidate set";
}

TEST_F(RandomDecisionGeneratorTest, GeneratePicksAmongManyActiveGoals) {
    auto goals = make_goals(kLargeGoalCount);
    auto ptrs = goal_ptrs(goals);
    candidates.insert(0);
    std::set<const goal_lineage*> chosen;
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .Times(kCoverageTrials)
        .WillRepeatedly([&] { return many_goals(ptrs); });
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable)
        .Times(kCoverageTrials)
        .WillRepeatedly(ReturnRef(candidates));
    EXPECT_CALL(lp, make_resolution_lineage(_, 0))
        .Times(kCoverageTrials)
        .WillRepeatedly(stub_make_resolution_lineage(resolutions));
    for (size_t i = 0; i < kCoverageTrials; ++i) {
        const resolution_lineage* result = generator.generate();
        EXPECT_EQ(result->idx, 0);
        EXPECT_TRUE(std::find(ptrs.begin(), ptrs.end(), result->parent) != ptrs.end());
        chosen.insert(result->parent);
    }
    EXPECT_GE(chosen.size(), kLargeGoalCount / 2)
        << "expected broad coverage across large active goal set";
}

TEST_F(RandomDecisionGeneratorTest, GeneratePicksAmongManyGoalsAndCandidates) {
    auto goals = make_goals(kLargeGoalCount);
    auto ptrs = goal_ptrs(goals);
    fill_candidates(candidates, kLargeCandidateCount);
    std::set<const goal_lineage*> chosen_goals;
    std::set<rule_id> chosen_rules;
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .Times(kCoverageTrials)
        .WillRepeatedly([&] { return many_goals(ptrs); });
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable)
        .Times(kCoverageTrials)
        .WillRepeatedly(ReturnRef(candidates));
    EXPECT_CALL(lp, make_resolution_lineage(_, _))
        .Times(kCoverageTrials)
        .WillRepeatedly(stub_make_resolution_lineage(resolutions));
    for (size_t i = 0; i < kCoverageTrials; ++i) {
        const resolution_lineage* result = generator.generate();
        EXPECT_TRUE(std::find(ptrs.begin(), ptrs.end(), result->parent) != ptrs.end());
        EXPECT_LT(result->idx, kLargeCandidateCount);
        chosen_goals.insert(result->parent);
        chosen_rules.insert(result->idx);
    }
    EXPECT_GE(chosen_goals.size(), kLargeGoalCount / 2)
        << "expected broad goal coverage when both dimensions are large";
    EXPECT_GE(chosen_rules.size(), kLargeCandidateCount / 2)
        << "expected broad rule coverage when both dimensions are large";
}
