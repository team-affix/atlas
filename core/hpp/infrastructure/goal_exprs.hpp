#ifndef GOAL_EXPRS_HPP
#define GOAL_EXPRS_HPP

#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

struct goal_exprs {
    framed_expr get(const goal_lineage*) const;
    void set(const goal_lineage*, framed_expr);
    void unset(const goal_lineage*);
    void clear_goal_exprs();
private:
    std::unordered_map<const goal_lineage*, framed_expr> exprs_;
};

#endif
