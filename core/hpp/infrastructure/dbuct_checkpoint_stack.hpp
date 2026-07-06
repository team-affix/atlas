#ifndef DBUCT_CHECKPOINT_STACK_HPP
#define DBUCT_CHECKPOINT_STACK_HPP

#include <cstddef>
#include "debug_assert.hpp"

// Trail translator for the delayed-backtracking solver. Every camped store is
// trail-backed, so this stack copies no state; it maps DBUCT's episode lifecycle
// onto the trail's frame push/pop (supplied as IPushFrame / IPopFrame). mark_root
// opens a persistent root frame enclosing an episode's leading unit propagation;
// each tree-policy choose() and the rollout phase each open one more frame; and
// terminate()'s reported backstep count becomes frame pops. frames_open_ tracks
// all frames since mark_root so restore_root() rewinds to the baseline without
// querying the trail's depth. Durable CDCL learning lives outside the trail.
template<typename IPushFrame, typename IPopFrame>
struct dbuct_checkpoint_stack {
    dbuct_checkpoint_stack(IPushFrame& push_frame, IPopFrame& pop_frame);

    void mark_root();

    void restore_root();

    void push_tree_policy();

    void enter_rollout();

    void end_episode(size_t steps);

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
