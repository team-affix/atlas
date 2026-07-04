#ifndef DBUCT_GOAL_EXPRS_HPP
#define DBUCT_GOAL_EXPRS_HPP

#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

// Delayed-backtracking (DBUCT) variant of goal_exprs.
//
// Behaviourally identical to goal_exprs, but exposes snapshot()/restore() so the
// dbuct checkpoint machinery can restore the full goal-expr map to any camped
// choice boundary. The DBUCT solver never clears this wholesale per sim; it is
// carried across episodes and rolled back exactly by checkpoint restoration.
struct dbuct_goal_exprs {
    using snapshot_t = std::unordered_map<const goal_lineage*, framed_expr>;

    framed_expr get(const goal_lineage* gl) const;
    void set(const goal_lineage* gl, framed_expr fe);
    void unset(const goal_lineage* gl);

    snapshot_t snapshot() const;
    void restore(snapshot_t s);

private:
    std::unordered_map<const goal_lineage*, framed_expr> exprs_;
};

inline framed_expr dbuct_goal_exprs::get(const goal_lineage* gl) const { return exprs_.at(gl); }

inline void dbuct_goal_exprs::set(const goal_lineage* gl, framed_expr fe) {
    auto [_, inserted] = exprs_.insert({gl, fe});
    DEBUG_ASSERT(inserted);
}

inline void dbuct_goal_exprs::unset(const goal_lineage* gl) {
    auto erased = exprs_.erase(gl);
    DEBUG_ASSERT(erased == 1);
}

inline dbuct_goal_exprs::snapshot_t dbuct_goal_exprs::snapshot() const { return exprs_; }
inline void dbuct_goal_exprs::restore(snapshot_t s) { exprs_ = std::move(s); }

#endif
