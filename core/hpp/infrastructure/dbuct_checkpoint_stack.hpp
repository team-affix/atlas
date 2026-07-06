#ifndef DBUCT_CHECKPOINT_STACK_HPP
#define DBUCT_CHECKPOINT_STACK_HPP

#include <cstddef>
#include "debug_assert.hpp"

// Trail translator for the delayed-backtracking solver.
//
// Every camped store is trail-backed (its mutations journal a backtrackable
// operation), so incremental backtracking replaces the former O(state) full-copy
// checkpointing entirely. This stack no longer copies state; it maps DBUCT's
// episode lifecycle onto the trail's frame operations, supplied as the abstract
// IPushFrame / IPopFrame (not a concrete trail):
//
//   * initial-goal activation logs mutations before mark_root (below the root
//     episode frame), so restore_root() never undoes them — that is the root
//     frontier;
//   * mark_root() opens a persistent root episode frame that encloses an
//     episode's leading unit propagation (which runs before any tree-policy
//     choose(), so it would otherwise be logged frame-less and un-poppable);
//   * each tree-policy choose() opens one more frame (push_tree_policy); a
//     choice's downstream resolution mutations land in that frame;
//   * entering the rollout phase opens one more frame (enter_rollout);
//   * terminate()'s reported backstep count becomes frame pops (end_episode):
//     the rollout frame first, then `steps` tree frames.
//
// A backstep leaves the resume frame open, so cascading re-application of learned
// avoidances logs into it and is itself undone on any further backstep — exactly
// the behaviour a snapshot scheme reproduced by re-deriving eliminations after a
// full restore. Durable CDCL learning lives outside the trail, so it survives.
//
// frames_open_ tracks every frame opened since mark_root (the root episode frame
// plus any tree/rollout frames) so restore_root() can pop back to the baseline
// without querying the trail's depth.
template<typename IPushFrame, typename IPopFrame>
struct dbuct_checkpoint_stack {
    dbuct_checkpoint_stack(IPushFrame& push_frame, IPopFrame& pop_frame);

    // Open the persistent root episode frame. restore_root() rewinds to here.
    void mark_root();

    // Rewind past the root episode frame and drop all episode frames. One-shot:
    // used at exhaustion, after which the solve is finished.
    void restore_root();

    // One frame per tree-policy choose(); kept in lockstep with DBUCT.
    void push_tree_policy();

    // One frame opened when an episode transitions into its rollout phase.
    void enter_rollout();

    // Synchronise with terminate()'s reported backstep count: pop the rollout
    // frame (if any) then `steps` tree frames.
    void end_episode(size_t steps);

    // Additional lazy backtrack driven by cascading re-application: pop `steps`
    // more tree frames.
    void pop_and_restore(size_t steps);

    size_t frame_depth() const;

private:
    IPushFrame& push_frame_;
    IPopFrame& pop_frame_;
    size_t frames_open_ = 0;
    size_t tree_frames_ = 0;
    bool in_rollout_ = false;
};

template<typename IPushFrame, typename IPopFrame>
dbuct_checkpoint_stack<IPushFrame, IPopFrame>::dbuct_checkpoint_stack(IPushFrame& push_frame, IPopFrame& pop_frame)
    : push_frame_(push_frame), pop_frame_(pop_frame) {}

template<typename IPushFrame, typename IPopFrame>
void dbuct_checkpoint_stack<IPushFrame, IPopFrame>::mark_root() {
    push_frame_.push();
    frames_open_ = 1;
    tree_frames_ = 0;
    in_rollout_ = false;
}

template<typename IPushFrame, typename IPopFrame>
void dbuct_checkpoint_stack<IPushFrame, IPopFrame>::restore_root() {
    for (size_t i = 0; i < frames_open_; ++i)
        pop_frame_.pop();
    frames_open_ = 0;
    tree_frames_ = 0;
    in_rollout_ = false;
}

template<typename IPushFrame, typename IPopFrame>
void dbuct_checkpoint_stack<IPushFrame, IPopFrame>::push_tree_policy() {
    push_frame_.push();
    ++frames_open_;
    ++tree_frames_;
}

template<typename IPushFrame, typename IPopFrame>
void dbuct_checkpoint_stack<IPushFrame, IPopFrame>::enter_rollout() {
    push_frame_.push();
    ++frames_open_;
    in_rollout_ = true;
}

template<typename IPushFrame, typename IPopFrame>
void dbuct_checkpoint_stack<IPushFrame, IPopFrame>::end_episode(size_t steps) {
    if (in_rollout_) {
        pop_frame_.pop();
        --frames_open_;
        in_rollout_ = false;
    }
    pop_and_restore(steps);
}

template<typename IPushFrame, typename IPopFrame>
void dbuct_checkpoint_stack<IPushFrame, IPopFrame>::pop_and_restore(size_t steps) {
    DEBUG_ASSERT(steps <= tree_frames_);
    for (size_t i = 0; i < steps; ++i) {
        pop_frame_.pop();
        --frames_open_;
    }
    tree_frames_ -= steps;
}

template<typename IPushFrame, typename IPopFrame>
size_t dbuct_checkpoint_stack<IPushFrame, IPopFrame>::frame_depth() const { return tree_frames_; }

#endif
