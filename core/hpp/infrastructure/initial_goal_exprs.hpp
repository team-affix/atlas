#ifndef INITIAL_GOAL_EXPRS_HPP
#define INITIAL_GOAL_EXPRS_HPP

#include <vector>
#include "interfaces/i_get_initial_goal_count.hpp"
#include "interfaces/i_get_initial_goal_expr.hpp"
#include "interfaces/i_push_initial_goal_expr.hpp"

struct initial_goal_exprs
    : i_get_initial_goal_count
    , i_get_initial_goal_expr
    , i_push_initial_goal_expr {
    void push(const expr*) override;
    size_t count() const override;
    const expr* get(size_t idx) const override;
private:
    std::vector<const expr*> exprs_;
};

#endif
