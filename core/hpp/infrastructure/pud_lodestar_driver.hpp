#ifndef PUD_LODESTAR_DRIVER_HPP
#define PUD_LODESTAR_DRIVER_HPP

#include <cstddef>
#include <optional>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_lodestar_choice.hpp"
#include "value_objects/pud_unfold_outcome.hpp"

// Picks and makes one unfold. The rings pick the leaf: the non-conjecture ring gets a turn while fewer than the
// ratio of non-conjecture unfolds have happened since the last conjecture unfold, then the conjecture ring, then
// the non-conjecture ring if it has not had its turn. The goal chooser then picks which of the leaf's goals to
// unfold it on.
template<
    typename INextNonConjectureChoice,
    typename INextConjectureChoice,
    typename IChooseGoal,
    typename IUnfoldLeaf>
struct pud_lodestar_driver {
    pud_lodestar_driver(
        INextNonConjectureChoice& next_non_conjecture_choice,
        INextConjectureChoice& next_conjecture_choice,
        IChooseGoal& choose_goal,
        IUnfoldLeaf& unfold_leaf,
        size_t non_conjecture_unfolds_per_conjecture_unfold);
    pud_unfold_outcome unfold_one();
private:
    pud_unfold_outcome unfold_chosen(pud_global_node_id leaf);

    INextNonConjectureChoice& next_non_conjecture_choice_;
    INextConjectureChoice& next_conjecture_choice_;
    IChooseGoal& choose_goal_;
    IUnfoldLeaf& unfold_leaf_;
    size_t non_conjecture_unfolds_per_conjecture_unfold_;
    size_t non_conjecture_unfolds_since_conjecture_unfold_;
};

#define PUD_LODESTAR_DRIVER_TEMPLATE                                                                \
    template<typename INextNonConjectureChoice, typename INextConjectureChoice, typename IChooseGoal, \
             typename IUnfoldLeaf>
#define PUD_LODESTAR_DRIVER                                                                         \
    pud_lodestar_driver<INextNonConjectureChoice, INextConjectureChoice, IChooseGoal, IUnfoldLeaf>

PUD_LODESTAR_DRIVER_TEMPLATE
PUD_LODESTAR_DRIVER::pud_lodestar_driver(
    INextNonConjectureChoice& next_non_conjecture_choice,
    INextConjectureChoice& next_conjecture_choice,
    IChooseGoal& choose_goal,
    IUnfoldLeaf& unfold_leaf,
    size_t non_conjecture_unfolds_per_conjecture_unfold)
    : next_non_conjecture_choice_(next_non_conjecture_choice)
    , next_conjecture_choice_(next_conjecture_choice)
    , choose_goal_(choose_goal)
    , unfold_leaf_(unfold_leaf)
    , non_conjecture_unfolds_per_conjecture_unfold_(non_conjecture_unfolds_per_conjecture_unfold)
    , non_conjecture_unfolds_since_conjecture_unfold_(0) {}

PUD_LODESTAR_DRIVER_TEMPLATE
pud_unfold_outcome PUD_LODESTAR_DRIVER::unfold_one() {
    const bool non_conjecture_turn =
        non_conjecture_unfolds_since_conjecture_unfold_ < non_conjecture_unfolds_per_conjecture_unfold_;
    if (non_conjecture_turn) {
        const std::optional<pud_lodestar_choice> outside = next_non_conjecture_choice_.next();
        if (outside.has_value())
            return unfold_chosen(outside->leaf);
    }
    const std::optional<pud_lodestar_choice> inside = next_conjecture_choice_.next();
    if (inside.has_value())
        return unfold_chosen(inside->leaf);
    if (non_conjecture_turn)
        return pud_unfold_outcome::none;
    const std::optional<pud_lodestar_choice> outside = next_non_conjecture_choice_.next();
    if (!outside.has_value())
        return pud_unfold_outcome::none;
    return unfold_chosen(outside->leaf);
}

// Unfolds the dealt leaf on the goal the chooser picks, and counts the unfold toward the ratio.
PUD_LODESTAR_DRIVER_TEMPLATE
pud_unfold_outcome PUD_LODESTAR_DRIVER::unfold_chosen(pud_global_node_id leaf) {
    const pud_unfold_outcome outcome = unfold_leaf_.unfold(leaf, choose_goal_.choose(leaf));
    if (outcome == pud_unfold_outcome::on_focus)
        non_conjecture_unfolds_since_conjecture_unfold_ = 0;
    else
        ++non_conjecture_unfolds_since_conjecture_unfold_;
    return outcome;
}

#undef PUD_LODESTAR_DRIVER
#undef PUD_LODESTAR_DRIVER_TEMPLATE

#endif
