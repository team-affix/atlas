#ifndef DBUCT_SOLVER_HPP
#define DBUCT_SOLVER_HPP

#include <cstddef>
#include <optional>
#include "infrastructure/coroutine.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"

// Delayed-backtracking solver: the camping analogue of `solver`.
//
// Whereas `solver` restarts every episode from the root (set_up / run /
// tear_down), dbuct_solver activates the initial goals exactly once and then
// keeps a single DBUCT instance camped deep in the search tree across episodes:
//
//   setup once ─▶ ┌─ run one episode from the camped frontier
//                 │  yield its termination (frontier still live for the caller)
//                 │  learn the terminal path's lemma
//                 │  lazily backstep (DBUCT decides how far) ─ checkpoints restore
//                 │  re-apply learned avoidances to the resume frontier,
//                 │  cascading further backsteps while ancestors collapse
//                 └─ repeat
//
// Refutation matches the restarting contract: an episode (or a collapsed
// frontier) with zero decisions on its path means there is no branch left to
// explore, so the search is exhausted.
template<typename IActivateInitialGoalsOnce, typename IRunSim,
         typename IGetDecisionCount, typename IDeriveDecisionLemma,
         typename IComputeReward, typename ITerminate, typename ILearnAvoidance,
         typename ILearnReapply>
struct dbuct_solver {
    dbuct_solver(IActivateInitialGoalsOnce& activate_once, IRunSim& run_sim,
                 IGetDecisionCount& get_decision_count,
                 IDeriveDecisionLemma& derive_decision_lemma,
                 IComputeReward& compute_reward, ITerminate& terminate,
                 ILearnAvoidance& learn, ILearnReapply& learn_reapply)
        : activate_once_(activate_once), run_sim_(run_sim),
          get_decision_count_(get_decision_count),
          derive_decision_lemma_(derive_decision_lemma),
          compute_reward_(compute_reward), terminate_(terminate),
          learn_(learn), learn_reapply_(learn_reapply) {}

    coroutine<sim_termination, void> solve();

private:
    // Fixpoint re-application after a backstep. Returns true when the search is
    // refuted (a collapse reached a zero-decision, i.e. root-level, frontier).
    bool reapply_cascade();

    IActivateInitialGoalsOnce& activate_once_;
    IRunSim& run_sim_;
    IGetDecisionCount& get_decision_count_;
    IDeriveDecisionLemma& derive_decision_lemma_;
    IComputeReward& compute_reward_;
    ITerminate& terminate_;
    ILearnAvoidance& learn_;
    ILearnReapply& learn_reapply_;
};

template<typename IA, typename IRS, typename IGDC, typename IDL, typename ICR,
         typename IT, typename ILA, typename ILR>
coroutine<sim_termination, void>
dbuct_solver<IA, IRS, IGDC, IDL, ICR, IT, ILA, ILR>::solve() {
    // One-time activation of the initial goals and all of their candidates on the
    // root frame. Failure here means the initial goals are refuted outright.
    if (!activate_once_.activate_initial_goals_and_candidates()) {
        co_yield sim_termination::conflicted;
        co_return;
    }

    // Snapshot the root frontier so an exhausted search can restore the caller's
    // observable state (bindings, active goals) to root — matching the restarting
    // solver, whose final teardown leaves the frontier back at the initial goals.
    terminate_.mark_root();

    bool refuted = false;
    while (!refuted) {
        const sim_termination term = run_sim_.run();

        // Expose the terminal frontier (solution bindings and all) to the caller
        // BEFORE any backstep rolls it back.
        co_yield term;

        // A path with no decisions leaves no branch to explore. The search is
        // exhausted: restore the root frontier so the caller's post-refutation
        // view matches the restarting solver (no leftover solution bindings).
        if (get_decision_count_.count() == 0) {
            terminate_.restore_root();
            co_return;
        }

        // Learn from the terminal path, then lazily backstep. terminate() reports
        // how many choice frames DBUCT unwound and drives the matching checkpoint
        // restoration, landing us on the resume frontier.
        const lemma dl = derive_decision_lemma_.derive_decision_lemma();
        const double reward = compute_reward_.compute_mcts_reward();
        terminate_.terminate(reward);

        std::optional<const resolution_lineage*> forced = learn_.learn(dl);
        if (forced.has_value())
            learn_reapply_.route_elimination(forced.value());

        refuted = reapply_cascade();
    }

    // Exhausted via a collapsed (root-level) frontier: normalise the observable
    // state back to root, as the restarting solver does at final teardown.
    terminate_.restore_root();
}

template<typename IA, typename IRS, typename IGDC, typename IDL, typename ICR,
         typename IT, typename ILA, typename ILR>
bool dbuct_solver<IA, IRS, IGDC, IDL, ICR, IT, ILA, ILR>::reapply_cascade() {
    while (true) {
        if (!learn_reapply_.reapply_frontier())
            return false;  // frontier consistent; ready for the next episode

        // The resume frontier collapsed under re-applied learning. If it carries
        // no decisions it is the root frontier — the search is refuted.
        if (get_decision_count_.count() == 0)
            return true;

        // Otherwise backjump: capture the collapsed frontier's no-good, force at
        // least one lazy backstep, then re-apply at the shallower frontier.
        const lemma dl = derive_decision_lemma_.derive_decision_lemma();
        const double reward = compute_reward_.compute_mcts_reward();
        std::size_t steps = 0;
        while (steps == 0)
            steps = terminate_.terminate(reward);

        std::optional<const resolution_lineage*> forced = learn_.learn(dl);
        if (forced.has_value())
            learn_reapply_.route_elimination(forced.value());
    }
}

#endif
