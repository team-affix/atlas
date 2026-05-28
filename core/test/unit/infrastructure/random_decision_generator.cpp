// Random decision generator: picks a uniform random active goal and candidate rule,
// then interns resolution via lineage pool. With singleton goal/rule sets the choice
// is deterministic.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include <random>
#include "infrastructure/random_decision_generator.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_iterate_active_goals.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

state_machine<const goal_lineage*> single_goal(const goal_lineage* gl) {
    co_yield gl;
}

state_machine<const goal_lineage*> two_goals(
    const goal_lineage* g0, const goal_lineage* g1) {
    co_yield g0;
    co_yield g1;
}

}  // namespace

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id), (override));
};

struct MockIterateActiveGoals : public i_iterate_active_goals {
    MOCK_METHOD(state_machine<const goal_lineage*>, iterate_active_goals, (), (const, override));
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

    goal_lineage gl{nullptr, 0};
    goal_lineage gl_alt{nullptr, 1};
    resolution_lineage expected_rl{&gl, 0};
    rule_id_set candidates;
};

TEST_F(RandomDecisionGeneratorTest, GenerateResolvesChosenGoalAndRule) {
    static constexpr rule_id kRule = 0;
    candidates.insert(kRule);
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .WillOnce([&] { return single_goal(&gl); });
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable(&gl)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(lp, make_resolution_lineage(&gl, kRule)).WillOnce(Return(&expected_rl));
    EXPECT_EQ(generator.generate(), &expected_rl);
}

TEST_F(RandomDecisionGeneratorTest, GeneratePicksAmongMultipleCandidates) {
    candidates.insert(0);
    candidates.insert(1);
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .WillOnce([&] { return single_goal(&gl); });
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable(&gl)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(lp, make_resolution_lineage(&gl, _))
        .WillOnce(Invoke([&](const goal_lineage* goal, rule_id rid) {
            EXPECT_EQ(goal, &gl);
            EXPECT_TRUE(rid == 0 || rid == 1);
            return &expected_rl;
        }));
    EXPECT_EQ(generator.generate(), &expected_rl);
}

TEST_F(RandomDecisionGeneratorTest, GeneratePicksAmongMultipleActiveGoals) {
    candidates.insert(0);
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .WillOnce([&] { return two_goals(&gl, &gl_alt); });
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable).WillRepeatedly(ReturnRef(candidates));
    EXPECT_CALL(lp, make_resolution_lineage(_, 0))
        .WillOnce(Invoke([&](const goal_lineage* goal, rule_id) {
            EXPECT_TRUE(goal == &gl || goal == &gl_alt);
            return &expected_rl;
        }));
    EXPECT_EQ(generator.generate(), &expected_rl);
}
