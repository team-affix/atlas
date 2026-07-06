#ifndef DBUCT_FRONTIER_READY_HPP
#define DBUCT_FRONTIER_READY_HPP

// Delayed-backtracking replacement for the run loop's initial-goals activator.
// Under DBUCT the initial goals are activated once on the root frame and every
// episode resumes from the camped frontier, so this hook is a no-op that just
// reports the frontier is ready.
struct dbuct_frontier_ready {
    bool activate_initial_goals_and_candidates();
};

inline bool dbuct_frontier_ready::activate_initial_goals_and_candidates() { return true; }

#endif
