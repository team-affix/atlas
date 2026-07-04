#ifndef DBUCT_FRONTIER_READY_HPP
#define DBUCT_FRONTIER_READY_HPP

// Delayed-backtracking replacement for the initial-goals activator inside the
// per-episode run loop.
//
// The restarting solvers re-activate the initial goals at the start of every
// sim. Under DBUCT the initial goals (and all of their candidates) are activated
// exactly once on the root frame; every subsequent episode resumes from the
// camped frontier with those goals already present. So the run loop's
// "activate initial goals" hook is a no-op that simply reports the frontier is
// ready.
struct dbuct_frontier_ready {
    bool activate_initial_goals_and_candidates() { return true; }
};

#endif
