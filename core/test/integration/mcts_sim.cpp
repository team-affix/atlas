// Integration: mcts_sim with real visits/value tables + tree_walker.
// Asserts table retention across episodes (not mocked table behavior).

#include <gtest/gtest.h>
#include <random>
#include <vector>
#include "infrastructure/mcts_root_tree_node.hpp"
#include "infrastructure/mcts_sim.hpp"
#include "infrastructure/tree_walker.hpp"
#include "random_rollout.hpp"
#include "uniform_exploration_constant.hpp"
#include "uniform_value_delta.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"
#include "value_table.hpp"
#include "visits_table.hpp"

namespace {

using test_visits_t = monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map>;
using test_value_t = monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map>;
using test_rollout_t = monte_carlo::random_rollout<
    mcts_choice, std::mt19937, std::vector<mcts_choice>, std::vector<mcts_choice>>;
using test_value_delta_t = monte_carlo::uniform_value_delta<double>;
using test_exploration_constant_t = monte_carlo::uniform_exploration_constant<double>;
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
    test_exploration_constant_t,
    mcts_root_tree_node>;

struct MctsSimIntegrationTest : public ::testing::Test {
    static constexpr double kExplorationConstant = 1.414;

    test_visits_t visits;
    test_value_t values;
    tree_walker walker;
    std::mt19937 rng{7};
    test_rollout_t rollout{rng};
    test_value_delta_t value_delta;
    test_exploration_constant_t exploration_constant{kExplorationConstant};
    mcts_root_tree_node root;
    test_mcts_sim_t sim{
        visits, visits, values, values,
        walker, rollout, value_delta, exploration_constant, root};

    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
};

TEST_F(MctsSimIntegrationTest, EpisodeUpdatesRootVisitsAndRetainsAcrossSecondEpisode) {
    std::vector<mcts_choice> choices{mcts_choice{&gl0}, mcts_choice{&gl1}};
    const mcts_tree_node_id root_id = root.get_mcts_root_node();

    EXPECT_EQ(visits.get_visits(root_id), 0u);

    value_delta.set_value(1.0);
    sim.set_up();
    (void)sim.choose(choices);
    sim.tear_down();

    const size_t visits_after_first = visits.get_visits(root_id);
    EXPECT_GE(visits_after_first, 1u);
    EXPECT_NE(values.get_value(root_id), 0.0);

    value_delta.set_value(0.5);
    sim.set_up();
    (void)sim.choose(choices);
    sim.tear_down();

    EXPECT_GT(visits.get_visits(root_id), visits_after_first)
        << "second episode accumulates visits on retained tables";
}

}  // namespace
