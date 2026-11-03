#ifndef PUD_SEARCH_STEPPER_HPP
#define PUD_SEARCH_STEPPER_HPP

#include <cstddef>
#include "value_objects/pud_search_stop_reason.hpp"
#include "value_objects/pud_unfold_outcome.hpp"

// One tick of a PUD search session: report a pending registration conflict, else unfold until a solution
// is taken or a blocked leaf is refuted. Every unfold counts against the session's budget. When no tick
// is left the stepper stops and remembers why.
template<
    typename IClearSolution,
    typename ITakePendingConflict,
    typename ITakeSolution,
    typename IRefuteBlocked,
    typename IUnfoldOne,
    typename IListQueries>
struct pud_search_stepper {
    pud_search_stepper(
        IClearSolution& clear_solution,
        ITakePendingConflict& take_pending_conflict,
        ITakeSolution& take_solution,
        IRefuteBlocked& refute_blocked,
        IUnfoldOne& unfold_one,
        IListQueries& list_queries,
        size_t max_unfolds);
    bool step();
    pud_search_stop_reason stop_reason() const;
    size_t unfold_count() const;
private:
    IClearSolution& clear_solution_;
    ITakePendingConflict& take_pending_conflict_;
    ITakeSolution& take_solution_;
    IRefuteBlocked& refute_blocked_;
    IUnfoldOne& unfold_one_;
    IListQueries& list_queries_;
    size_t max_unfolds_;
    size_t unfolds_;
    pud_search_stop_reason stop_reason_;
};

#define PUD_SEARCH_STEPPER_TEMPLATE                                                        \
    template<typename IClearSolution, typename ITakePendingConflict, typename ITakeSolution,       \
             typename IRefuteBlocked, typename IUnfoldOne, typename IListQueries>
#define PUD_SEARCH_STEPPER                                                                 \
    pud_search_stepper<IClearSolution, ITakePendingConflict, ITakeSolution, IRefuteBlocked, \
        IUnfoldOne, IListQueries>

PUD_SEARCH_STEPPER_TEMPLATE
PUD_SEARCH_STEPPER::pud_search_stepper(
    IClearSolution& clear_solution,
    ITakePendingConflict& take_pending_conflict,
    ITakeSolution& take_solution,
    IRefuteBlocked& refute_blocked,
    IUnfoldOne& unfold_one,
    IListQueries& list_queries,
    size_t max_unfolds)
    : clear_solution_(clear_solution)
    , take_pending_conflict_(take_pending_conflict)
    , take_solution_(take_solution)
    , refute_blocked_(refute_blocked)
    , unfold_one_(unfold_one)
    , list_queries_(list_queries)
    , max_unfolds_(max_unfolds)
    , unfolds_(0)
    , stop_reason_(pud_search_stop_reason::running) {}

PUD_SEARCH_STEPPER_TEMPLATE
bool PUD_SEARCH_STEPPER::step() {
    clear_solution_.clear();
    if (stop_reason_ != pud_search_stop_reason::running)
        return false;
    if (take_pending_conflict_.try_take())
        return true;
    for (;;) {
        if (take_solution_.take())
            return true;
        if (refute_blocked_.refute_blocked())
            return true;
        if (unfolds_ >= max_unfolds_) {
            stop_reason_ = pud_search_stop_reason::budget_spent;
            return false;
        }
        if (unfold_one_.unfold_one() == pud_unfold_outcome::none) {
            const bool focus_open = !list_queries_.list().empty();
            stop_reason_ = focus_open ? pud_search_stop_reason::exhausted : pud_search_stop_reason::refuted;
            return false;
        }
        ++unfolds_;
    }
}

PUD_SEARCH_STEPPER_TEMPLATE
pud_search_stop_reason PUD_SEARCH_STEPPER::stop_reason() const {
    return stop_reason_;
}

PUD_SEARCH_STEPPER_TEMPLATE
size_t PUD_SEARCH_STEPPER::unfold_count() const {
    return unfolds_;
}

#undef PUD_SEARCH_STEPPER
#undef PUD_SEARCH_STEPPER_TEMPLATE

#endif
