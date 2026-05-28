// MCTS decision generator: builds choice vectors from active goals and candidate
// rules, delegates to monte_carlo::simulation, then interns the resolution. With a
// single choice at each stage the outcome is fully determined.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include <random>
#include "infrastructure/mcts_decision_generator.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_iterate_active_goals.hpp"
#include "interfaces/i_active_goals_size.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "mcts.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

namespace {

state_machine<const goal_lineage*> single_goal(const goal_lineage* gl) {
    co_yield gl;
}

}  // namespace

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id), (override));
};

struct MockIterateActiveGoals : public i_iterate_active_goals {
    MOCK_METHOD(state_machine<const goal_lineage*>, iterate_active_goals, (), (const, override));
};

struct MockActiveGoalsSize : public i_active_goals_size {
    MOCK_METHOD(size_t, active_goals_size, (), (const, override));
};

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_id_set&, get, (const goal_lineage*), (const, override));
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
    resolution_lineage expected_rl{&gl, 0};
    rule_id_set candidates;
};

TEST_F(MctsDecisionGeneratorTest, GenerateResolvesChosenGoalAndRule) {
    static constexpr rule_id kRule = 0;
    candidates.insert(kRule);
    EXPECT_CALL(active_goals_size, active_goals_size()).WillOnce(Return(1));
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .WillOnce([&] { return single_goal(&gl); });
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&gl)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(lp, make_resolution_lineage(&gl, kRule)).WillOnce(Return(&expected_rl));
    EXPECT_EQ(generator.generate(), &expected_rl);
}
