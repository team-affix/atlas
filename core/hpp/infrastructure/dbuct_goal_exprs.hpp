#ifndef DBUCT_GOAL_EXPRS_HPP
#define DBUCT_GOAL_EXPRS_HPP

#include <memory>
#include <unordered_map>
#include "infrastructure/backtrackable_map_erase.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/framed_expr.hpp"

// Delayed-backtracking (DBUCT) variant of goal_exprs.
//
// Behaviourally identical to goal_exprs, but its mutations are journalled on the
// shared trail: set/unset log a backtrackable map insert/erase so a camped choice
// frame rolls the goal-expr map back exactly when the trail pops. The DBUCT solver
// never clears this wholesale per sim; it is carried across episodes and rewound
// incrementally by trail pops rather than by a full-copy checkpoint.
struct dbuct_goal_exprs {
    using map_t = std::unordered_map<const goal_lineage*, framed_expr>;

    explicit dbuct_goal_exprs(trail& t);

    framed_expr get(const goal_lineage* gl) const;
    void set(const goal_lineage* gl, framed_expr fe);
    void unset(const goal_lineage* gl);

private:
    tracked<map_t, trail> exprs_;
};

inline dbuct_goal_exprs::dbuct_goal_exprs(trail& t) : exprs_(t, map_t{}) {}

inline framed_expr dbuct_goal_exprs::get(const goal_lineage* gl) const { return exprs_.get().at(gl); }

inline void dbuct_goal_exprs::set(const goal_lineage* gl, framed_expr fe) {
    exprs_.mutate(std::make_unique<backtrackable_map_insert<map_t>>(gl, fe));
}

inline void dbuct_goal_exprs::unset(const goal_lineage* gl) {
    exprs_.mutate(std::make_unique<backtrackable_map_erase<map_t>>(gl));
}

#endif
