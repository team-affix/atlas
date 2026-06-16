#ifndef GOAL_EXPRS_HPP
#define GOAL_EXPRS_HPP

#include <unordered_map>
#include "interfaces/i_get_goal_expr.hpp"
#include "interfaces/i_set_goal_expr.hpp"
#include "interfaces/i_unset_goal_expr.hpp"
#include "interfaces/i_clear_goal_exprs.hpp"
#include "value_objects/framed_expr.hpp"

struct goal_exprs
    : i_get_goal_expr
    , i_set_goal_expr
    , i_unset_goal_expr
    , i_clear_goal_exprs {
    framed_expr get(const goal_lineage*) const override;
    void set(const goal_lineage*, framed_expr) override;
    void unset(const goal_lineage*) override;
    void clear_goal_exprs() override;
private:
    std::unordered_map<const goal_lineage*, framed_expr> exprs_;
};

#endif
