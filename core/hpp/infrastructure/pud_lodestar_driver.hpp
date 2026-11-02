#ifndef PUD_LODESTAR_DRIVER_HPP
#define PUD_LODESTAR_DRIVER_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/pud_lodestar_choice.hpp"
#include "value_objects/pud_leaf_goal_query.hpp"
#include "value_objects/pud_unfold_outcome.hpp"

// Picks and makes one unfold. A unit query is unfolded first. Otherwise the non-conjecture ring gets
// a turn while fewer than the ratio of non-conjecture unfolds have happened since the last conjecture
// unfold, then the conjecture ring, then the non-conjecture ring if it has not had its turn.
template<
    typename IPropagateUnit,
    typename IListQueries,
    typename IBuildRing,
    typename INextNonConjectureChoice,
    typename INextConjectureChoice,
    typename IUnfoldLeaf>
struct pud_lodestar_driver {
    pud_lodestar_driver(
        IPropagateUnit& propagate_unit,
        IListQueries& list_queries,
        IBuildRing& build_ring,
        INextNonConjectureChoice& next_non_conjecture_choice,
        INextConjectureChoice& next_conjecture_choice,
        IUnfoldLeaf& unfold_leaf,
        size_t non_conjecture_unfolds_per_conjecture_unfold);
    pud_unfold_outcome unfold_one();
private:
    pud_unfold_outcome count(pud_unfold_outcome outcome);

    IPropagateUnit& propagate_unit_;
    IListQueries& list_queries_;
    IBuildRing& build_ring_;
    INextNonConjectureChoice& next_non_conjecture_choice_;
    INextConjectureChoice& next_conjecture_choice_;
    IUnfoldLeaf& unfold_leaf_;
    size_t non_conjecture_unfolds_per_conjecture_unfold_;
    size_t non_conjecture_unfolds_since_conjecture_unfold_;
};

#define PUD_LODESTAR_DRIVER_TEMPLATE                                                                \
    template<typename IPropagateUnit, typename IListQueries, typename IBuildRing,                  \
             typename INextNonConjectureChoice, typename INextConjectureChoice, typename IUnfoldLeaf>
#define PUD_LODESTAR_DRIVER                                                                         \
    pud_lodestar_driver<IPropagateUnit, IListQueries, IBuildRing, INextNonConjectureChoice,        \
        INextConjectureChoice, IUnfoldLeaf>

PUD_LODESTAR_DRIVER_TEMPLATE
PUD_LODESTAR_DRIVER::pud_lodestar_driver(
    IPropagateUnit& propagate_unit,
    IListQueries& list_queries,
    IBuildRing& build_ring,
    INextNonConjectureChoice& next_non_conjecture_choice,
    INextConjectureChoice& next_conjecture_choice,
    IUnfoldLeaf& unfold_leaf,
    size_t non_conjecture_unfolds_per_conjecture_unfold)
    : propagate_unit_(propagate_unit)
    , list_queries_(list_queries)
    , build_ring_(build_ring)
    , next_non_conjecture_choice_(next_non_conjecture_choice)
    , next_conjecture_choice_(next_conjecture_choice)
    , unfold_leaf_(unfold_leaf)
    , non_conjecture_unfolds_per_conjecture_unfold_(non_conjecture_unfolds_per_conjecture_unfold)
    , non_conjecture_unfolds_since_conjecture_unfold_(0) {}

PUD_LODESTAR_DRIVER_TEMPLATE
pud_unfold_outcome PUD_LODESTAR_DRIVER::unfold_one() {
    const pud_unfold_outcome unit = propagate_unit_.propagate();
    if (unit != pud_unfold_outcome::none)
        return count(unit);
    const std::vector<pud_leaf_goal_query> queries = list_queries_.list();
    const bool non_conjecture_turn =
        non_conjecture_unfolds_since_conjecture_unfold_ < non_conjecture_unfolds_per_conjecture_unfold_;
    if (non_conjecture_turn) {
        const std::optional<pud_lodestar_choice> outside =
            next_non_conjecture_choice_.next(build_ring_.build(queries, false));
        if (outside.has_value())
            return count(unfold_leaf_.unfold(outside->leaf, outside->body_goal));
    }
    const std::optional<pud_lodestar_choice> inside = next_conjecture_choice_.next(build_ring_.build(queries, true));
    if (inside.has_value())
        return count(unfold_leaf_.unfold(inside->leaf, inside->body_goal));
    if (non_conjecture_turn)
        return pud_unfold_outcome::none;
    const std::optional<pud_lodestar_choice> outside =
        next_non_conjecture_choice_.next(build_ring_.build(queries, false));
    if (!outside.has_value())
        return pud_unfold_outcome::none;
    return count(unfold_leaf_.unfold(outside->leaf, outside->body_goal));
}

PUD_LODESTAR_DRIVER_TEMPLATE
pud_unfold_outcome PUD_LODESTAR_DRIVER::count(pud_unfold_outcome outcome) {
    if (outcome == pud_unfold_outcome::on_focus)
        non_conjecture_unfolds_since_conjecture_unfold_ = 0;
    else
        ++non_conjecture_unfolds_since_conjecture_unfold_;
    return outcome;
}

#undef PUD_LODESTAR_DRIVER
#undef PUD_LODESTAR_DRIVER_TEMPLATE

#endif
