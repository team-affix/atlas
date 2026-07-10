#ifndef DBUCT_SOLVER_HPP
#define DBUCT_SOLVER_HPP

#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/elimination_result.hpp"

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
//
// terminate() reports the 0-based frame index it backtracked TO; index 0 is the
// root. That index is the only "are we at the root" signal -- the solver keeps no
// base offset of its own -- and it drives both refutation (a root run with zero
// decisions) and the cascade's stop conditions.
template<typename IActivateInitialGoalsOnce, typename IRunSim,
         typename IGetDecisionCount, typename IComputeReward,
         typename ITerminate, typename ILearnAvoidance,
         typename IMhu, typename IEliminationRouter, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal>
struct dbuct_solver {
    dbuct_solver(IActivateInitialGoalsOnce& activate_once, IRunSim& run_sim,
                 IGetDecisionCount& get_decision_count,
                 IComputeReward& compute_reward, ITerminate& terminate,
                 ILearnAvoidance& learn, IMhu& mhu, IEliminationRouter& router,
                 IConflictDetector& conflict_detector,
                 IUnitGoalDetector& unit_goal_detector,
                 IPushUnitGoal& push_unit_goal)
        : activate_once_(activate_once), run_sim_(run_sim),
          get_decision_count_(get_decision_count),
          compute_reward_(compute_reward), terminate_(terminate),
          learn_(learn), mhu_(mhu), router_(router),
          conflict_detector_(conflict_detector),
          unit_goal_detector_(unit_goal_detector),
          push_unit_goal_(push_unit_goal) {}

    coroutine<sim_termination, void> solve();

private:
    // Learn the current terminal conflict, backtrack, route the emitted
    // eliminations, and cascade further learn/backtrack rounds while the resume
    // frontier keeps collapsing. Returns whether the cascade left us at the root
    // (the next episode's "runs from root" flag). Refutation is decided by the
    // top-level loop (a root-frontier run with no decisions), never here.
    bool learn_terminate_cascade(bool conflicted);

    // Route one pop_frame elimination at the resume frontier. Forced eliminations
    // surfaced by pop_frame bypass the joint eliminator, so they carry the same
    // obligations the joint applies to CDCL constrain hits: drop the lineage's
    // MHU head, deactivate the candidate through the shared elimination router,
    // and -- when a goal fully collapses -- fire conflict / unit-goal detection.
    // Returns whether a conflict was realized.
    bool route_elimination(const resolution_lineage* rl);
    bool route_eliminations(const std::vector<const resolution_lineage*>& elims);

    IActivateInitialGoalsOnce& activate_once_;
    IRunSim& run_sim_;
    IGetDecisionCount& get_decision_count_;
    IComputeReward& compute_reward_;
    ITerminate& terminate_;
    ILearnAvoidance& learn_;
    IMhu& mhu_;
    IEliminationRouter& router_;
    IConflictDetector& conflict_detector_;
    IUnitGoalDetector& unit_goal_detector_;
    IPushUnitGoal& push_unit_goal_;
};

template<typename IA, typename IRS, typename IGDC, typename ICR,
         typename IT, typename ILA, typename IM, typename IER,
         typename ICD, typename IUGD, typename IPUG>
coroutine<sim_termination, void>
dbuct_solver<IA, IRS, IGDC, ICR, IT, ILA, IM, IER, ICD, IUGD, IPUG>::solve() {
    if (!activate_once_.activate_initial_goals_and_candidates()) {
        co_yield sim_termination::conflicted;
        co_return;
    }

    // The base frame is the root, so the first episode runs from the root.
    bool at_root = true;

    while (true) {
        // Whether this episode runs from the root frontier (no camped decisions
        // above it). dbuct backtracks at trail-frame granularity, which is FINER
        // than a decision (each generate() makes several tree-policy chooses: one
        // per goal-navigation step plus the candidate), so being off the root
        // does NOT imply a decision is recorded. Only a run that STARTS at the
        // root and records zero decisions proves the search exhausted -- this is
        // exactly the restarting solver's `count()==0` test, always evaluated
        // from the root.
        const bool from_root = at_root;

        const sim_termination term = run_sim_.run();

        co_yield term;

        if (from_root && get_decision_count_.count() == 0) {
            // Search exhausted: unwind the episode's work so the refuted session
            // is left in the clean root state (query vars normalize to unbound).
            terminate_.unwind_to_root();
            break;
        }

        at_root = learn_terminate_cascade(term == sim_termination::conflicted);
    }
}

template<typename IA, typename IRS, typename IGDC, typename ICR,
         typename IT, typename ILA, typename IM, typename IER,
         typename ICD, typename IUGD, typename IPUG>
bool dbuct_solver<IA, IRS, IGDC, ICR, IT, ILA, IM, IER, ICD, IUGD, IPUG>::learn_terminate_cascade(bool conflicted) {
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
    auto result = terminate_.terminate(reward, true);
    // Backstep to the next genuine branch point: a frontier that both drops a
    // decision from the terminated path AND still carries at least one decision
    // to explore. Stopping short of that leaves a spent or deterministic frontier
    // in place -- either one whose recorded decisions are all intact (re-derives
    // the same path) or a decision-free tail that unit-propagates to the very
    // solution the root already yields (a duplicate solved tick). Collapse all
    // the way to the root when no shallower branch point survives.
    while (result.frame_index > 0 &&
           !(get_decision_count_.count() < decisions_before &&
             get_decision_count_.count() > 0)) {
        const double r = compute_reward_.compute_mcts_reward();
        result = terminate_.terminate(r, true);
    }

    // Route the pop_frame eliminations at the resume frontier; if that realizes a
    // fresh conflict, learn + backtrack again and repeat until the frontier is
    // stable or we have collapsed all the way back to the root.
    while (route_eliminations(result.eliminations)) {
        if (result.frame_index == 0)
            break;
        const double r = compute_reward_.compute_mcts_reward();
        learn_.learn();
        result = terminate_.terminate(r, true);
    }

    return result.frame_index == 0;
}

template<typename IA, typename IRS, typename IGDC, typename ICR,
         typename IT, typename ILA, typename IM, typename IER,
         typename ICD, typename IUGD, typename IPUG>
bool dbuct_solver<IA, IRS, IGDC, ICR, IT, ILA, IM, IER, ICD, IUGD, IPUG>::route_elimination(const resolution_lineage* rl) {
    mhu_.remove_head(rl);
    const elimination_result res = router_.route(rl);
    if (res != elimination_result::eliminated)
        return false;
    const goal_lineage* gl = rl->parent;
    if (conflict_detector_.detect(gl))
        return true;
    if (unit_goal_detector_.detect(gl))
        push_unit_goal_.push(gl);
    return false;
}

template<typename IA, typename IRS, typename IGDC, typename ICR,
         typename IT, typename ILA, typename IM, typename IER,
         typename ICD, typename IUGD, typename IPUG>
bool dbuct_solver<IA, IRS, IGDC, ICR, IT, ILA, IM, IER, ICD, IUGD, IPUG>::route_eliminations(
    const std::vector<const resolution_lineage*>& elims) {
    bool conflict = false;
    for (const resolution_lineage* rl : elims) {
        if (route_elimination(rl))
            conflict = true;
    }
    return conflict;
}

#endif
