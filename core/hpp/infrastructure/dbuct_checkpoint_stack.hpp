#ifndef DBUCT_CHECKPOINT_STACK_HPP
#define DBUCT_CHECKPOINT_STACK_HPP

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

// Full per-sim state checkpointing for the delayed-backtracking solver.
//
// DBUCT camps deep in the search tree and backtracks lazily: terminate() reports
// how many tree-policy choice frames it unwound, and the caller must restore its
// own state to the corresponding resume node. This stack keeps one full state
// snapshot per tree-policy choice frame, held in lockstep with DBUCT's internal
// frame stack (one push_tree_policy() per tree-policy choose()), plus a single
// transient snapshot for the rollout phase.
//
// Restoration is exact: a snapshot captures the entire mutable per-sim frontier
// (goal exprs, candidate sets, active-goal tree, frame offsets, chosen
// candidates, unit queue, decision memory, bump cursor, common bind_map, MHU,
// and the elimination backlog). Popping N frames and re-installing the shallowest
// popped snapshot reconstructs the frontier as it stood before those N choices —
// including the MHU heads and common substitutions that a delta-based scheme
// could not cheaply undo.
//
// This is the maximally faithful realisation of delayed backtracking; the O(state)
// cost per choice frame is the obvious first target for a future incremental
// (journalled) optimisation, but correctness here is independent of that.
template<typename GoalExprs, typename GCR, typename SRT, typename CFO,
         typename CGC, typename UG, typename DM, typename FBA,
         typename BindMap, typename Mhu, typename EB, typename RM>
struct dbuct_checkpoint_stack {
    struct checkpoint {
        typename GoalExprs::snapshot_t goal_exprs;
        typename GCR::snapshot_t       gcr;
        typename SRT::snapshot_t       srt;
        typename CFO::snapshot_t       cfo;
        typename CGC::snapshot_t       cgc;
        typename UG::snapshot_t        ug;
        typename DM::snapshot_t        dm;
        typename FBA::snapshot_t       fba;
        typename BindMap::snapshot_t   bind_map;
        typename Mhu::snapshot_t       mhu;   // move-only
        typename EB::snapshot_t        eb;
        typename RM::snapshot_t        rm;
    };

    dbuct_checkpoint_stack(GoalExprs& ge, GCR& gcr, SRT& srt, CFO& cfo, CGC& cgc,
                           UG& ug, DM& dm, FBA& fba, BindMap& bm, Mhu& mhu, EB& eb,
                           RM& rm)
        : ge_(ge), gcr_(gcr), srt_(srt), cfo_(cfo), cgc_(cgc), ug_(ug), dm_(dm),
          fba_(fba), bm_(bm), mhu_(mhu), eb_(eb), rm_(rm) {}

    // Capture the root frontier (initial goals + all candidates, no resolutions
    // yet), taken once after one-time activation. This is the only snapshot of the
    // pre-episode state when a solve makes zero tree-policy choices (a purely
    // unit-propagated solution pushes no frames), so it is what restores the
    // observable frontier to root when such a search is exhausted.
    void mark_root() { root_.emplace(capture()); }

    // Restore the root frontier and drop all episode state. One-shot: used at
    // exhaustion, after which the solve is finished.
    void restore_root() {
        if (root_.has_value())
            apply(std::move(*root_));
        root_.reset();
        frames_.clear();
        rollout_.reset();
        in_rollout_ = false;
    }

    // One snapshot per tree-policy choose(); kept in lockstep with DBUCT's stack.
    void push_tree_policy() { frames_.push_back(capture()); }

    // Captured once when an episode transitions into its rollout phase.
    void enter_rollout() {
        rollout_.emplace(capture());
        in_rollout_ = true;
    }

    // Synchronise with terminate()'s reported backstep count and drop the
    // rollout snapshot. When steps > 0 the shallowest popped frame is a full
    // snapshot taken before rollout began, so it subsumes the rollout state too.
    void end_episode(size_t steps) {
        if (steps > 0) {
            pop_and_restore(steps);
        } else if (in_rollout_) {
            apply(std::move(*rollout_));
        }
        rollout_.reset();
        in_rollout_ = false;
    }

    // Additional lazy backtrack driven by cascading re-application (a learned
    // avoidance that empties an ancestor frontier). Pops `steps` more choice
    // frames and restores the resume node.
    void pop_and_restore(size_t steps) {
        if (steps == 0) return;
        checkpoint cp;
        for (size_t i = 0; i < steps; ++i) {
            cp = std::move(frames_.back());
            frames_.pop_back();
        }
        apply(std::move(cp));
    }

    size_t frame_depth() const { return frames_.size(); }

private:
    checkpoint capture() const {
        return checkpoint{
            ge_.snapshot(),  gcr_.snapshot(), srt_.snapshot(), cfo_.snapshot(),
            cgc_.snapshot(), ug_.snapshot(),  dm_.snapshot(),  fba_.snapshot(),
            bm_.snapshot(),  mhu_.snapshot(), eb_.snapshot(),  rm_.snapshot()};
    }

    void apply(checkpoint cp) {
        ge_.restore(std::move(cp.goal_exprs));
        gcr_.restore(std::move(cp.gcr));
        srt_.restore(std::move(cp.srt));
        cfo_.restore(std::move(cp.cfo));
        cgc_.restore(std::move(cp.cgc));
        ug_.restore(std::move(cp.ug));
        dm_.restore(std::move(cp.dm));
        fba_.restore(std::move(cp.fba));
        bm_.restore(std::move(cp.bind_map));
        mhu_.restore(std::move(cp.mhu));
        eb_.restore(std::move(cp.eb));
        rm_.restore(std::move(cp.rm));
    }

    GoalExprs& ge_;
    GCR&       gcr_;
    SRT&       srt_;
    CFO&       cfo_;
    CGC&       cgc_;
    UG&        ug_;
    DM&        dm_;
    FBA&       fba_;
    BindMap&   bm_;
    Mhu&       mhu_;
    EB&        eb_;
    RM&        rm_;

    std::vector<checkpoint>   frames_;
    std::optional<checkpoint> rollout_;
    std::optional<checkpoint> root_;
    bool                      in_rollout_ = false;
};

#endif
