#ifndef PUD_SEARCHER_HPP
#define PUD_SEARCHER_HPP

#include <cstddef>
#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_search_step.hpp"
#include "value_objects/pud_search_stop_reason.hpp"
#include "value_objects/pud_unfold_outcome.hpp"

// A PUD search session as a coroutine that yields after every step: each unfold, each reported solution,
// each reported refutation. Every unfold counts against the session's budget. When no step is left the
// coroutine finishes and the searcher remembers why: exhausted while a focus leaf is still live, else refuted.
template<
    typename IClearSolution,
    typename ITakePendingConflict,
    typename ITakeSolution,
    typename IUnfoldOne,
    typename IGetLiveFocusLeaves>
struct pud_searcher {
    pud_searcher(
        IClearSolution& clear_solution,
        ITakePendingConflict& take_pending_conflict,
        ITakeSolution& take_solution,
        IUnfoldOne& unfold_one,
        IGetLiveFocusLeaves& get_live_focus_leaves,
        size_t max_unfolds);
    coroutine<pud_search_step, void> search();
    pud_search_stop_reason stop_reason() const;
    size_t unfold_count() const;
private:
    IClearSolution& clear_solution_;
    ITakePendingConflict& take_pending_conflict_;
    ITakeSolution& take_solution_;
    IUnfoldOne& unfold_one_;
    IGetLiveFocusLeaves& get_live_focus_leaves_;
    size_t max_unfolds_;
    size_t unfolds_;
    pud_search_stop_reason stop_reason_;
};

#define PUD_SEARCHER_TEMPLATE                                                                       \
    template<typename IClearSolution, typename ITakePendingConflict, typename ITakeSolution,       \
             typename IUnfoldOne, typename IGetLiveFocusLeaves>
#define PUD_SEARCHER                                                                                \
    pud_searcher<IClearSolution, ITakePendingConflict, ITakeSolution, IUnfoldOne, IGetLiveFocusLeaves>

PUD_SEARCHER_TEMPLATE
PUD_SEARCHER::pud_searcher(
    IClearSolution& clear_solution,
    ITakePendingConflict& take_pending_conflict,
    ITakeSolution& take_solution,
    IUnfoldOne& unfold_one,
    IGetLiveFocusLeaves& get_live_focus_leaves,
    size_t max_unfolds)
    : clear_solution_(clear_solution)
    , take_pending_conflict_(take_pending_conflict)
    , take_solution_(take_solution)
    , unfold_one_(unfold_one)
    , get_live_focus_leaves_(get_live_focus_leaves)
    , max_unfolds_(max_unfolds)
    , unfolds_(0)
    , stop_reason_(pud_search_stop_reason::running) {}

PUD_SEARCHER_TEMPLATE
coroutine<pud_search_step, void> PUD_SEARCHER::search() {
    for (;;) {
        clear_solution_.clear();
        if (take_pending_conflict_.try_take()) {
            co_yield pud_search_step::refuted;
            continue;
        }
        if (take_solution_.take()) {
            co_yield pud_search_step::solved;
            continue;
        }
        if (unfolds_ >= max_unfolds_) {
            stop_reason_ = pud_search_stop_reason::budget_spent;
            co_return;
        }
        if (unfold_one_.unfold_one() == pud_unfold_outcome::none) {
            const bool focus_open = !get_live_focus_leaves_.live().empty();
            stop_reason_ = focus_open ? pud_search_stop_reason::exhausted : pud_search_stop_reason::refuted;
            co_return;
        }
        ++unfolds_;
        co_yield pud_search_step::unfolded;
    }
}

PUD_SEARCHER_TEMPLATE
pud_search_stop_reason PUD_SEARCHER::stop_reason() const {
    return stop_reason_;
}

PUD_SEARCHER_TEMPLATE
size_t PUD_SEARCHER::unfold_count() const {
    return unfolds_;
}

#undef PUD_SEARCHER
#undef PUD_SEARCHER_TEMPLATE

#endif
