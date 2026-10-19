#ifndef PUD_RANDOM_SUBGOAL_CHOOSER_HPP
#define PUD_RANDOM_SUBGOAL_CHOOSER_HPP

#include <algorithm>
#include <random>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

template<typename IGetNodeDescent>
struct pud_random_subgoal_chooser {
    pud_random_subgoal_chooser(IGetNodeDescent& get_node_descent, std::mt19937& rng);
    pud_body_goal_id choose_subgoal(pud_node_id leaf);
private:
    IGetNodeDescent& get_node_descent_;
    std::mt19937& rng_;
};

template<typename IGetNodeDescent>
pud_random_subgoal_chooser<IGetNodeDescent>::pud_random_subgoal_chooser(
    IGetNodeDescent& get_node_descent,
    std::mt19937& rng)
    : get_node_descent_(get_node_descent)
    , rng_(rng) {}

template<typename IGetNodeDescent>
pud_body_goal_id pud_random_subgoal_chooser<IGetNodeDescent>::choose_subgoal(pud_node_id leaf) {
    std::vector<pud_body_goal_id> open_goals;
    for (const auto& [goal, goal_expr] : get_node_descent_.get(leaf).body_goal_expr().body_goal_exprs())
        open_goals.push_back(goal);
    DEBUG_ASSERT(!open_goals.empty());
    std::sort(open_goals.begin(), open_goals.end());
    std::uniform_int_distribution<size_t> pick(0, open_goals.size() - 1);
    return open_goals.at(pick(rng_));
}

#endif
