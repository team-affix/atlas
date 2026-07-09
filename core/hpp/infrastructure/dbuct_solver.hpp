#ifndef DBUCT_SOLVER_HPP
#define DBUCT_SOLVER_HPP

#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/sim_termination.hpp"

// Delayed-backtracking solver: the camping analogue of `solver`. Activates the
// initial goals once, then keeps a single DBUCT instance camped across episodes.
// Each episode runs from the camped frontier and yields its termination. When it
// reaches a terminal state with decisions still on the path, the solver learns
// the terminal conflict FIRST (at the still-terminal, deepest frame), then
// backtracks via terminate() -- whose first CDCL pop_frame emits the forced
// elimination for the conflict just learned. Those pop_frame eliminations are
// routed at the resume frontier (dropping MHU heads and deactivating candidates);
// if routing collapses the resume frontier into a fresh conflict, the solver
// learns + terminates again, cascading backjumps until the frontier is stable or
// a zero-decision path proves the search exhausted (refuted).
template<typename IActivateInitialGoalsOnce, typename IRunSim,
         typename IGetDecisionCount, typename IComputeReward,
         typename ITerminate, typename ILearnAvoidance, typename IRouteEliminations>
struct dbuct_solver {
    dbuct_solver(IActivateInitialGoalsOnce& activate_once, IRunSim& run_sim,
                 IGetDecisionCount& get_decision_count,
                 IComputeReward& compute_reward, ITerminate& terminate,
                 ILearnAvoidance& learn, IRouteEliminations& route_eliminations)
        : activate_once_(activate_once), run_sim_(run_sim),
          get_decision_count_(get_decision_count),
          compute_reward_(compute_reward), terminate_(terminate),
          learn_(learn), route_eliminations_(route_eliminations) {}

    coroutine<sim_termination, void> solve();

private:
    // Learn the current terminal conflict, backtrack, route the emitted
    // eliminations, and cascade further learn/backtrack rounds while the resume
    // frontier keeps collapsing. Refutation is decided by the top-level loop (a
    // root-frontier run with no decisions), never here.
    void learn_terminate_cascade(bool conflicted);

    IActivateInitialGoalsOnce& activate_once_;
    IRunSim& run_sim_;
    IGetDecisionCount& get_decision_count_;
    IComputeReward& compute_reward_;
    ITerminate& terminate_;
    ILearnAvoidance& learn_;
    IRouteEliminations& route_eliminations_;
};

template<typename IA, typename IRS, typename IGDC, typename ICR,
         typename IT, typename ILA, typename IRE>
coroutine<sim_termination, void>
dbuct_solver<IA, IRS, IGDC, ICR, IT, ILA, IRE>::solve() {
    if (!activate_once_.activate_initial_goals_and_candidates()) {
        co_yield sim_termination::conflicted;
        co_return;
    }

    terminate_.mark_root();

    while (true) {
        // Whether this episode runs from the root frontier (no camped decisions
        // above it). dbuct backtracks at trail-frame granularity, which is FINER
        // than a decision (each generate() makes several tree-policy chooses: one
        // per goal-navigation step plus the candidate), so a nonzero camp depth
        // does NOT imply a decision is recorded. Only a run that STARTS at the
        // root and records zero decisions proves the search exhausted -- this is
        // exactly the restarting solver's `count()==0` test, which is always
        // evaluated from the root.
        const bool from_root = terminate_.camp_depth() == 0;

        const sim_termination term = run_sim_.run();

        co_yield term;

        if (from_root && get_decision_count_.count() == 0)
            break;

        learn_terminate_cascade(term == sim_termination::conflicted);
    }

    terminate_.restore_root();
}

template<typename IA, typename IRS, typename IGDC, typename ICR,
         typename IT, typename ILA, typename IRE>
void dbuct_solver<IA, IRS, IGDC, ICR, IT, ILA, IRE>::learn_terminate_cascade(bool conflicted) {
    // Fixed adjacency: record the terminal conflict, then immediately backtrack;
    // terminate()'s first pop_frame yields the forced elimination for it. Nothing
    // may sit between learn() and terminate(). Every terminal -- conflict OR
    // solution -- must pop at least one frame: a spent frontier left in place is
    // re-detected verbatim by the next episode, re-reporting the same solution
    // (or re-deriving the same conflict) and never applying the avoidance just
    // learned. Forcing one pop lets that first pop_frame surface the avoidance
    // and advances the camped path toward the root.
    (void)conflicted;
    const std::size_t decisions_before = get_decision_count_.count();
    const double reward = compute_reward_.compute_mcts_reward();
    learn_.learn();
    std::vector<const resolution_lineage*> elims = terminate_.terminate(reward, true);
    // Backstep to the next genuine branch point: a frontier that both drops a
    // decision from the terminated path AND still carries at least one decision
    // to explore. Stopping short of that leaves a spent or deterministic frontier
    // in place -- either one whose recorded decisions are all intact (re-derives
    // the same path) or a decision-free tail that unit-propagates to the very
    // solution the root already yields (a duplicate solved tick). Collapse all
    // the way to the root when no shallower branch point survives.
    while (terminate_.camp_depth() > 0 &&
           !(get_decision_count_.count() < decisions_before &&
             get_decision_count_.count() > 0)) {
        const double r = compute_reward_.compute_mcts_reward();
        elims = terminate_.terminate(r, true);
    }

    // Route the pop_frame eliminations at the resume frontier; if that realizes a
    // fresh conflict, learn + backtrack again and repeat until the frontier is
    // stable or we have collapsed all the way back to the root.
    while (route_eliminations_.route_eliminations(elims)) {
        if (terminate_.camp_depth() == 0)
            break;
        const double r = compute_reward_.compute_mcts_reward();
        learn_.learn();
        elims = terminate_.terminate(r, true);
    }
}

#endif
