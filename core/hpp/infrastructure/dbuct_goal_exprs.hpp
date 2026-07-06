#ifndef DBUCT_GOAL_EXPRS_HPP
#define DBUCT_GOAL_EXPRS_HPP

#include <memory>
#include <unordered_map>
#include "infrastructure/backtrackable_map_erase.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/framed_expr.hpp"

// Delayed-backtracking variant of goal_exprs. set/unset journal a backtrackable
// map insert/erase on the trail (abstracted as ILogTrailAction), so a camped
// choice frame rewinds the goal-expr map incrementally on trail pop rather than
// via a full-copy checkpoint.
template<typename ILogTrailAction>
struct dbuct_goal_exprs {
    using map_t = std::unordered_map<const goal_lineage*, framed_expr>;

    explicit dbuct_goal_exprs(ILogTrailAction& t);

    framed_expr get(const goal_lineage* gl) const;
    void set(const goal_lineage* gl, framed_expr fe);
    void unset(const goal_lineage* gl);

private:
    tracked<map_t, ILogTrailAction> exprs_;
};

template<typename ILogTrailAction>
dbuct_goal_exprs<ILogTrailAction>::dbuct_goal_exprs(ILogTrailAction& t) : exprs_(t, map_t{}) {}

template<typename ILogTrailAction>
framed_expr dbuct_goal_exprs<ILogTrailAction>::get(const goal_lineage* gl) const { return exprs_.get().at(gl); }

template<typename ILogTrailAction>
void dbuct_goal_exprs<ILogTrailAction>::set(const goal_lineage* gl, framed_expr fe) {
    exprs_.mutate(std::make_unique<backtrackable_map_insert<map_t>>(gl, fe));
}

template<typename ILogTrailAction>
void dbuct_goal_exprs<ILogTrailAction>::unset(const goal_lineage* gl) {
    exprs_.mutate(std::make_unique<backtrackable_map_erase<map_t>>(gl));
}

#endif
