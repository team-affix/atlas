#include "infrastructure/pud_random_unfold_chooser.hpp"
#include <iterator>
#include "debug_assert.hpp"

pud_random_unfold_chooser::pud_random_unfold_chooser(std::mt19937& rng)
    : rng_(rng) {}

pud_unfold_choice pud_random_unfold_chooser::choose_unfold(
    const std::map<pud_derivation_id, std::vector<pud_body_goal_id>>& frontier) {
    DEBUG_ASSERT(!frontier.empty());
    std::uniform_int_distribution<size_t> pick_derivation(0, frontier.size() - 1);
    const auto& [derivation, goals] = *std::next(frontier.begin(), pick_derivation(rng_));
    DEBUG_ASSERT(!goals.empty());
    std::uniform_int_distribution<size_t> pick_goal(0, goals.size() - 1);
    return pud_unfold_choice{.derivation = derivation, .goal = goals.at(pick_goal(rng_))};
}
