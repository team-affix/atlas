#ifndef DBUCT_SOLVER_HPP
#define DBUCT_SOLVER_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"

// Delayed-backtracking solver: the camping analogue of `solver`. Activates the
// initial goals once, then keeps a single DBUCT instance camped across episodes —
// each episode runs from the camped frontier, yields its termination, learns the
// terminal lemma, lazily backsteps (checkpoints restore), and re-applies learned
// avoidances to the resume frontier, cascading further backsteps as ancestors
// collapse. A zero-decision path means the search is exhausted (refuted).
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
    if (!activate_once_.activate_initial_goals_and_candidates()) {
        co_yield sim_termination::conflicted;
        co_return;
    }

    terminate_.mark_root();

    bool refuted = false;
    while (!refuted) {
        const sim_termination term = run_sim_.run();

        co_yield term;

        if (get_decision_count_.count() == 0) {
            terminate_.restore_root();
            co_return;
        }

        const lemma dl = derive_decision_lemma_.derive_decision_lemma();
        const double reward = compute_reward_.compute_mcts_reward();
        terminate_.terminate(reward);

        learn_.learn(dl);

        refuted = reapply_cascade();
    }

    terminate_.restore_root();
}

template<typename IA, typename IRS, typename IGDC, typename IDL, typename ICR,
         typename IT, typename ILA, typename ILR>
bool dbuct_solver<IA, IRS, IGDC, IDL, ICR, IT, ILA, ILR>::reapply_cascade() {
    while (true) {
        if (!learn_reapply_.reapply_frontier())
            return false;

        if (get_decision_count_.count() == 0)
            return true;

        const lemma dl = derive_decision_lemma_.derive_decision_lemma();
        const double reward = compute_reward_.compute_mcts_reward();
        // terminate() reports the frame index backtracked TO; index 0 (root) is a
        // valid outcome, not a retry signal, so a single call syncs the frontier.
        terminate_.terminate(reward);

        learn_.learn(dl);
    }
}

#endif
