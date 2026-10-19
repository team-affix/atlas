#ifndef PUD_RANDOM_UNFOLD_CHOOSER_HPP
#define PUD_RANDOM_UNFOLD_CHOOSER_HPP

#include <iterator>
#include <optional>
#include <random>
#include <set>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_derivation_id.hpp"
#include "value_objects/pud_open_goal.hpp"
#include "value_objects/pud_unfold_choice.hpp"
#include "debug_assert.hpp"

template<typename IIterateBodyGoals>
struct pud_random_unfold_chooser {
    pud_random_unfold_chooser(IIterateBodyGoals& iterate_body_goals, std::mt19937& rng);
    pud_unfold_choice choose_unfold(const std::set<pud_derivation_id>& frontier);
private:
    IIterateBodyGoals& iterate_body_goals_;
    std::mt19937& rng_;
};

template<typename IIBG>
pud_random_unfold_chooser<IIBG>::pud_random_unfold_chooser(IIBG& iterate_body_goals, std::mt19937& rng)
    : iterate_body_goals_(iterate_body_goals)
    , rng_(rng) {}

template<typename IIBG>
pud_unfold_choice pud_random_unfold_chooser<IIBG>::choose_unfold(const std::set<pud_derivation_id>& frontier) {
    DEBUG_ASSERT(!frontier.empty());
    std::uniform_int_distribution<size_t> pick_derivation(0, frontier.size() - 1);
    const pud_derivation_id derivation = *std::next(frontier.begin(), pick_derivation(rng_));
    std::vector<pud_body_goal_id> goals;
    coroutine<pud_open_goal, void> body_goals = iterate_body_goals_.iterate_body_goals(derivation);
    while (const std::optional<pud_open_goal> goal = body_goals.next())
        goals.push_back(goal->goal);
    DEBUG_ASSERT(!goals.empty());
    std::uniform_int_distribution<size_t> pick_goal(0, goals.size() - 1);
    return pud_unfold_choice{.derivation = derivation, .goal = goals.at(pick_goal(rng_))};
}

#endif
