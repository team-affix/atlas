#ifndef PUD_LODESTAR_SEARCH_STEPPER_HPP
#define PUD_LODESTAR_SEARCH_STEPPER_HPP

#include <cstddef>

template<
    typename IClearSolution,
    typename ICountSolutions,
    typename IGetPendingConflicts,
    typename ISetPendingConflicts,
    typename IGetStopped,
    typename ISetStopped,
    typename IGetMaxUnfolds,
    typename IGetUnfolds,
    typename ISetUnfolds,
    typename IGetFocusUnfolds,
    typename ISetFocusUnfolds,
    typename ITakeSolution,
    typename IRefuteBlocked,
    typename IUnfoldOne,
    typename ICheckLastUnfoldOnFocus>
struct pud_lodestar_search_stepper {
    pud_lodestar_search_stepper(
        IClearSolution& clear_solution,
        ICountSolutions& count_solutions,
        IGetPendingConflicts& get_pending_conflicts,
        ISetPendingConflicts& set_pending_conflicts,
        IGetStopped& get_stopped,
        ISetStopped& set_stopped,
        IGetMaxUnfolds& get_max_unfolds,
        IGetUnfolds& get_unfolds,
        ISetUnfolds& set_unfolds,
        IGetFocusUnfolds& get_focus_unfolds,
        ISetFocusUnfolds& set_focus_unfolds,
        ITakeSolution& take_solution,
        IRefuteBlocked& refute_blocked,
        IUnfoldOne& unfold_one,
        ICheckLastUnfoldOnFocus& check_last_unfold_on_focus);
    bool step();
private:
    IClearSolution& clear_solution_;
    ICountSolutions& count_solutions_;
    IGetPendingConflicts& get_pending_conflicts_;
    ISetPendingConflicts& set_pending_conflicts_;
    IGetStopped& get_stopped_;
    ISetStopped& set_stopped_;
    IGetMaxUnfolds& get_max_unfolds_;
    IGetUnfolds& get_unfolds_;
    ISetUnfolds& set_unfolds_;
    IGetFocusUnfolds& get_focus_unfolds_;
    ISetFocusUnfolds& set_focus_unfolds_;
    ITakeSolution& take_solution_;
    IRefuteBlocked& refute_blocked_;
    IUnfoldOne& unfold_one_;
    ICheckLastUnfoldOnFocus& check_last_unfold_on_focus_;
};

#define PUD_LODESTAR_SEARCH_STEPPER_TEMPLATE                                                        \
    template<typename IClearSolution, typename ICountSolutions, typename IGetPendingConflicts,     \
             typename ISetPendingConflicts, typename IGetStopped, typename ISetStopped,             \
             typename IGetMaxUnfolds, typename IGetUnfolds, typename ISetUnfolds,                  \
             typename IGetFocusUnfolds, typename ISetFocusUnfolds, typename ITakeSolution,          \
             typename IRefuteBlocked, typename IUnfoldOne, typename ICheckLastUnfoldOnFocus>
#define PUD_LODESTAR_SEARCH_STEPPER                                                                 \
    pud_lodestar_search_stepper<IClearSolution, ICountSolutions, IGetPendingConflicts,            \
        ISetPendingConflicts, IGetStopped, ISetStopped, IGetMaxUnfolds, IGetUnfolds, ISetUnfolds,  \
        IGetFocusUnfolds, ISetFocusUnfolds, ITakeSolution, IRefuteBlocked, IUnfoldOne,             \
        ICheckLastUnfoldOnFocus>

PUD_LODESTAR_SEARCH_STEPPER_TEMPLATE
PUD_LODESTAR_SEARCH_STEPPER::pud_lodestar_search_stepper(
    IClearSolution& clear_solution,
    ICountSolutions& count_solutions,
    IGetPendingConflicts& get_pending_conflicts,
    ISetPendingConflicts& set_pending_conflicts,
    IGetStopped& get_stopped,
    ISetStopped& set_stopped,
    IGetMaxUnfolds& get_max_unfolds,
    IGetUnfolds& get_unfolds,
    ISetUnfolds& set_unfolds,
    IGetFocusUnfolds& get_focus_unfolds,
    ISetFocusUnfolds& set_focus_unfolds,
    ITakeSolution& take_solution,
    IRefuteBlocked& refute_blocked,
    IUnfoldOne& unfold_one,
    ICheckLastUnfoldOnFocus& check_last_unfold_on_focus)
    : clear_solution_(clear_solution)
    , count_solutions_(count_solutions)
    , get_pending_conflicts_(get_pending_conflicts)
    , set_pending_conflicts_(set_pending_conflicts)
    , get_stopped_(get_stopped)
    , set_stopped_(set_stopped)
    , get_max_unfolds_(get_max_unfolds)
    , get_unfolds_(get_unfolds)
    , set_unfolds_(set_unfolds)
    , get_focus_unfolds_(get_focus_unfolds)
    , set_focus_unfolds_(set_focus_unfolds)
    , take_solution_(take_solution)
    , refute_blocked_(refute_blocked)
    , unfold_one_(unfold_one)
    , check_last_unfold_on_focus_(check_last_unfold_on_focus) {}

PUD_LODESTAR_SEARCH_STEPPER_TEMPLATE
bool PUD_LODESTAR_SEARCH_STEPPER::step() {
    clear_solution_.clear();
    if (get_stopped_.get_stopped())
        return false;
    const size_t pending_conflicts = get_pending_conflicts_.get();
    if (pending_conflicts > 0) {
        set_pending_conflicts_.set(pending_conflicts - 1);
        return true;
    }
    for (;;) {
        if (take_solution_.take())
            return true;
        if (refute_blocked_.refute_blocked())
            return true;
        const bool budget_spent = get_focus_unfolds_.get_focus_unfolds() >= get_max_unfolds_.get_max_unfolds();
        if (budget_spent) {
            set_stopped_.set_stopped(true);
            return count_solutions_.solution_count() == 0;
        }
        if (!unfold_one_.unfold_one()) {
            set_stopped_.set_stopped(true);
            return false;
        }
        set_unfolds_.set_unfolds(get_unfolds_.get_unfolds() + 1);
        if (!check_last_unfold_on_focus_.last_unfold_on_focus())
            continue;
        set_focus_unfolds_.set_focus_unfolds(get_focus_unfolds_.get_focus_unfolds() + 1);
    }
}

#undef PUD_LODESTAR_SEARCH_STEPPER
#undef PUD_LODESTAR_SEARCH_STEPPER_TEMPLATE

#endif
