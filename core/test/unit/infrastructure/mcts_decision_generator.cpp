// MCTS decision generator: builds choice vectors from active goals and candidate
// rules, delegates to monte_carlo::simulation, then interns the resolution. With a
// single choice at each stage the outcome is fully determined.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include "locator_fixture.hpp"
#include "infrastructure/mcts_decision_generator.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_iterate_active_goals.hpp"
#include "interfaces/i_active_goals_size.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "mcts.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

coroutine<const goal_lineage*, void> single_goal(const goal_lineage* gl) {
    co_yield gl;
}

coroutine<const goal_lineage*, void> two_goals(
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
    MOCK_METHOD((coroutine<const goal_lineage*, void>), iterate_active_goals, (), (const, override));
};

struct MockActiveGoalsSize : public i_active_goals_size {
    MOCK_METHOD(size_t, active_goals_size, (), (const, override));
};

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get_mutable, (const goal_lineage*), ());
    MOCK_METHOD(const i_rule_id_set&, get_const, (const goal_lineage*), (const));
    i_rule_id_set& get(const goal_lineage* gl) override { return get_mutable(gl); }
    const i_rule_id_set& get(const goal_lineage* gl) const override { return get_const(gl); }
};

struct MctsDecisionGeneratorTest : public ::testing::Test {
    locator loc;
    MockMakeResolutionLineage lp;
    MockIterateActiveGoals iterate_active_goals;
    MockActiveGoalsSize active_goals_size;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    monte_carlo::tree_node<mcts_choice> root;
    std::mt19937 rng{0};
    monte_carlo::simulation<mcts_choice, std::mt19937> mcts_sim{root, 1.0, rng};
    mcts_decision_generator generator;

    MctsDecisionGeneratorTest() : generator(init_generator()) {}

    mcts_decision_generator init_generator() {
        loc.bind_as<i_make_resolution_lineage>(lp);
        loc.bind_as<i_iterate_active_goals>(iterate_active_goals);
        loc.bind_as<i_active_goals_size>(active_goals_size);
        loc.bind_as<i_get_goal_candidate_rule_ids>(get_goal_candidate_rule_ids);
        return mcts_decision_generator{loc, mcts_sim};
    }

    goal_lineage gl{nullptr, 0};
    goal_lineage gl_alt{nullptr, 1};
    resolution_lineage expected_rl{&gl, 0};
    rule_id_set candidates;
    rule_id_set candidates_alt;
};

TEST_F(MctsDecisionGeneratorTest, GenerateResolvesChosenGoalAndRule) {
    static constexpr rule_id kRule = 0;
    candidates.insert(kRule);
    EXPECT_CALL(active_goals_size, active_goals_size()).WillOnce(Return(1));
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .WillOnce([&] { return single_goal(&gl); });
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable(&gl)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(lp, make_resolution_lineage(&gl, kRule)).WillOnce(Return(&expected_rl));
    EXPECT_EQ(generator.generate(), &expected_rl);
}

TEST_F(MctsDecisionGeneratorTest, GeneratePicksAmongMultipleGoalsAndCandidates) {
    candidates.insert(0);
    candidates.insert(1);
    candidates_alt.insert(0);
    EXPECT_CALL(active_goals_size, active_goals_size()).WillOnce(Return(2));
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .WillOnce([&] { return two_goals(&gl, &gl_alt); });
    EXPECT_CALL(get_goal_candidate_rule_ids, get_mutable)
        .WillRepeatedly([&](const goal_lineage* goal) -> i_rule_id_set& {
            if (goal == &gl)
                return candidates;
            EXPECT_EQ(goal, &gl_alt);
            return candidates_alt;
        });
    EXPECT_CALL(lp, make_resolution_lineage(_, _))
        .WillOnce(Invoke([&](const goal_lineage* goal, rule_id rid) {
            EXPECT_TRUE(goal == &gl || goal == &gl_alt);
            if (goal == &gl)
                EXPECT_TRUE(rid == 0 || rid == 1);
            else
                EXPECT_EQ(rid, 0);
            return &expected_rl;
        }));
    EXPECT_EQ(generator.generate(), &expected_rl);
}
