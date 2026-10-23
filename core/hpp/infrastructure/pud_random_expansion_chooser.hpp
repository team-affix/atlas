#ifndef PUD_RANDOM_EXPANSION_CHOOSER_HPP
#define PUD_RANDOM_EXPANSION_CHOOSER_HPP

#include <cstddef>
#include <iterator>
#include <random>
#include <set>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_unfold.hpp"
#include "debug_assert.hpp"

// Chooses an open goal of a frontier leaf, uniformly at random at each level.
template<typename IGetNodeGoals>
struct pud_random_expansion_chooser {
    pud_random_expansion_chooser(IGetNodeGoals& get_node_goals, std::mt19937& rng);
    pud_unfold choose_expansion(const std::set<pud_node_id>& frontier);
private:
    IGetNodeGoals& get_node_goals_;
    std::mt19937& rng_;
};

template<typename IGNG>
pud_random_expansion_chooser<IGNG>::pud_random_expansion_chooser(IGNG& get_node_goals, std::mt19937& rng)
    : get_node_goals_(get_node_goals)
    , rng_(rng) {}

template<typename IGNG>
pud_unfold pud_random_expansion_chooser<IGNG>::choose_expansion(const std::set<pud_node_id>& frontier) {
    DEBUG_ASSERT(!frontier.empty());
    std::uniform_int_distribution<size_t> pick_leaf(0, frontier.size() - 1);
    const pud_node_id leaf = *std::next(frontier.begin(), pick_leaf(rng_));
    const auto& goals = get_node_goals_.get(leaf).goals;
    DEBUG_ASSERT(!goals.empty());
    std::uniform_int_distribution<size_t> pick_goal(0, goals.size() - 1);
    return pud_unfold{.leaf = leaf, .goal = goals.at(pick_goal(rng_)).id};
}

#endif
