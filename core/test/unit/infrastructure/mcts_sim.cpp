// mcts_sim: orchestrates injected Monte Carlo collaborators.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <vector>
#include "infrastructure/mcts_sim.hpp"
#include "infrastructure/mcts_root_tree_node.hpp"
#include "infrastructure/tree_walker.hpp"
#include "uniform_value_delta.hpp"
#include "value_table.hpp"
#include "visits_table.hpp"
#include "random_rollout.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"

using test_visits_t = monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map>;
using test_value_t = monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map>;
using test_rollout_t = monte_carlo::random_rollout<
    mcts_choice, std::mt19937, std::vector<mcts_choice>, std::vector<mcts_choice>>;
using test_value_delta_t = monte_carlo::uniform_value_delta<double>;
using test_mcts_sim_t = mcts_sim<
    mcts_tree_node_id,
    mcts_choice,
    test_visits_t,
    test_visits_t,
    test_value_t,
    test_value_t,
    tree_walker,
    test_rollout_t,
    test_value_delta_t,
    mcts_root_tree_node>;

struct MctsSimTest : public ::testing::Test {
    static constexpr double kExplorationConstant = 1.414;

    test_visits_t visits;
    test_value_t values;
    tree_walker walker;
    std::mt19937 rng{42};
    test_rollout_t rollout{rng};
    test_value_delta_t value_delta;
    mcts_root_tree_node root;
    test_mcts_sim_t sim{
        visits, visits, values, values,
        walker, rollout, value_delta, root, kExplorationConstant};

    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
};

TEST_F(MctsSimTest, SetUpThenTearDownRuns) {
    value_delta.set_value(-1.0);
    sim.set_up();
    sim.tear_down();
}

TEST_F(MctsSimTest, ChooseAfterSetUpReturnsOneOfSuppliedGoalChoices) {
    sim.set_up();
    std::vector<mcts_choice> choices{mcts_choice{&gl0}, mcts_choice{&gl1}};
    mcts_choice picked = sim.choose(choices, false);
    const auto* picked_gl = std::get_if<const goal_lineage*>(&picked);
    ASSERT_NE(picked_gl, nullptr);
    EXPECT_TRUE(*picked_gl == &gl0 || *picked_gl == &gl1);
    value_delta.set_value(0.0);
    sim.tear_down();
}

TEST_F(MctsSimTest, ChooseAfterSetUpReturnsOneOfSuppliedRuleChoices) {
    sim.set_up();
    std::vector<mcts_choice> choices{mcts_choice{rule_id{0}}, mcts_choice{rule_id{1}}};
    mcts_choice picked = sim.choose(choices, false);
    const auto* picked_rule = std::get_if<rule_id>(&picked);
    ASSERT_NE(picked_rule, nullptr);
    EXPECT_TRUE(*picked_rule == 0 || *picked_rule == 1);
    value_delta.set_value(0.0);
    sim.tear_down();
}

TEST_F(MctsSimTest, MultipleRolloutChoosesStayWithinChoiceSet) {
    sim.set_up();
    std::vector<mcts_choice> choices{mcts_choice{&gl0}, mcts_choice{&gl1}};
    for (int i = 0; i < 10; ++i) {
        mcts_choice picked = sim.choose(choices, false);
        const auto* picked_gl = std::get_if<const goal_lineage*>(&picked);
        ASSERT_NE(picked_gl, nullptr);
        EXPECT_TRUE(*picked_gl == &gl0 || *picked_gl == &gl1);
    }
    value_delta.set_value(0.0);
    sim.tear_down();
}

TEST_F(MctsSimTest, FullLifecycleSetUpChooseTearDownTwice) {
    std::vector<mcts_choice> choices{mcts_choice{rule_id{0}}, mcts_choice{rule_id{1}}};
    for (int cycle = 0; cycle < 2; ++cycle) {
        sim.set_up();
        mcts_choice picked = sim.choose(choices, false);
        EXPECT_TRUE(std::holds_alternative<rule_id>(picked));
        value_delta.set_value(-static_cast<double>(cycle));
        sim.tear_down();
    }
}
