#ifndef DBUCT_CHECKPOINT_STACK_HPP
#define DBUCT_CHECKPOINT_STACK_HPP

#include <cstddef>
#include "debug_assert.hpp"
#include "infrastructure/trail.hpp"

// Trail translator for the delayed-backtracking solver.
//
// Every camped store is trail-backed (its mutations journal a backtrackable
// operation), so incremental backtracking replaces the former O(state) full-copy
// checkpointing entirely. This stack no longer copies state; it maps DBUCT's
// episode lifecycle onto the shared trail:
//
//   * initial-goal activation logs mutations with no enclosing frame (below the
//     root baseline), so restore_root() never undoes them — that is the root
//     frontier;
//   * each tree-policy choose() opens one trail frame (push_tree_policy); a
//     choice's downstream resolution mutations land in that frame;
//   * entering the rollout phase opens one more frame (enter_rollout);
//   * terminate()'s reported backstep count becomes trail pops (end_episode):
//     the rollout frame first, then `steps` tree frames.
//
// A backstep leaves the resume frame open, so cascading re-application of learned
// avoidances logs into it and is itself undone on any further backstep — exactly
// the behaviour a snapshot scheme reproduced by re-deriving eliminations after a
// full restore. Durable CDCL learning lives outside the trail, so it survives.
struct dbuct_checkpoint_stack {
    explicit dbuct_checkpoint_stack(trail& t);

    // Baseline the root frontier (initial goals + all candidates, no resolutions
    // yet), taken once after one-time activation. restore_root() rewinds to here.
    void mark_root();

    // Rewind to the root frontier and drop all episode frames. One-shot: used at
    // exhaustion, after which the solve is finished.
    void restore_root();

    // One trail frame per tree-policy choose(); kept in lockstep with DBUCT.
    void push_tree_policy();

    // One trail frame opened when an episode transitions into its rollout phase.
    void enter_rollout();

    // Synchronise with terminate()'s reported backstep count: pop the rollout
    // frame (if any) then `steps` tree frames.
    void end_episode(size_t steps);

    // Additional lazy backtrack driven by cascading re-application: pop `steps`
    // more tree frames.
    void pop_and_restore(size_t steps);

    size_t frame_depth() const;

private:
    trail& trail_;
    size_t root_baseline_ = 0;
    size_t tree_frames_ = 0;
    bool in_rollout_ = false;
};

inline dbuct_checkpoint_stack::dbuct_checkpoint_stack(trail& t) : trail_(t) {}

inline void dbuct_checkpoint_stack::mark_root() {
    // Open a persistent root episode frame ABOVE the baseline. Initial-goal
    // activation logged its mutations below the baseline (they are the permanent
    // root frontier). But an episode's leading unit propagation runs before any
    // tree-policy choose(), so without this frame those resolution binds would be
    // logged frame-less and restore_root could not undo them (e.g. a zero-decision
    // unit solution must leave the goal vars unbound again once exhausted).
    root_baseline_ = trail_.depth();
    trail_.push();
    tree_frames_ = 0;
    in_rollout_ = false;
}

inline void dbuct_checkpoint_stack::restore_root() {
    while (trail_.depth() > root_baseline_)
        trail_.pop();
    tree_frames_ = 0;
    in_rollout_ = false;
}

inline void dbuct_checkpoint_stack::push_tree_policy() {
    trail_.push();
    ++tree_frames_;
}

inline void dbuct_checkpoint_stack::enter_rollout() {
    trail_.push();
    in_rollout_ = true;
}

inline void dbuct_checkpoint_stack::end_episode(size_t steps) {
    if (in_rollout_) {
        trail_.pop();
        in_rollout_ = false;
    }
    pop_and_restore(steps);
}

inline void dbuct_checkpoint_stack::pop_and_restore(size_t steps) {
    DEBUG_ASSERT(steps <= tree_frames_);
    for (size_t i = 0; i < steps; ++i)
        trail_.pop();
    tree_frames_ -= steps;
}

inline size_t dbuct_checkpoint_stack::frame_depth() const { return tree_frames_; }

#endif
