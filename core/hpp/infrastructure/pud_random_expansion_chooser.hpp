#ifndef PUD_RANDOM_EXPANSION_CHOOSER_HPP
#define PUD_RANDOM_EXPANSION_CHOOSER_HPP

#include <iterator>
#include <random>
#include <set>
#include <vector>
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/pud_derivation_id.hpp"
#include "value_objects/pud_expansion_choice.hpp"
#include "debug_assert.hpp"

template<typename IGetBodyGoals>
struct pud_random_expansion_chooser {
    pud_random_expansion_chooser(IGetBodyGoals& get_body_goals, std::mt19937& rng);
    pud_expansion_choice choose_expansion(const std::set<pud_derivation_id>& frontier);
private:
    IGetBodyGoals& get_body_goals_;
    std::mt19937& rng_;
};

template<typename IGBG>
pud_random_expansion_chooser<IGBG>::pud_random_expansion_chooser(IGBG& get_body_goals, std::mt19937& rng)
    : get_body_goals_(get_body_goals)
    , rng_(rng) {}

template<typename IGBG>
pud_expansion_choice pud_random_expansion_chooser<IGBG>::choose_expansion(
    const std::set<pud_derivation_id>& frontier) {
    DEBUG_ASSERT(!frontier.empty());
    std::uniform_int_distribution<size_t> pick_derivation(0, frontier.size() - 1);
    const pud_derivation_id derivation = *std::next(frontier.begin(), pick_derivation(rng_));
    const std::vector<pud_body_goal> goals = get_body_goals_.body_goals(derivation);
    DEBUG_ASSERT(!goals.empty());
    std::uniform_int_distribution<size_t> pick_goal(0, goals.size() - 1);
    return pud_expansion_choice{.derivation = derivation, .goal = goals.at(pick_goal(rng_)).id};
}

#endif
