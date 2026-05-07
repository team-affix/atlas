#ifndef INITIAL_GOAL_EXPR_INITIALIZER_HPP
#define INITIAL_GOAL_EXPR_INITIALIZER_HPP

#include <vector>
#include "../interfaces/i_initial_goal_expr_initializer.hpp"
#include "../interfaces/i_goal_expr_store.hpp"
#include "../value_objects/expr.hpp"

struct initial_goal_expr_initializer : i_initial_goal_expr_initializer {
    initial_goal_expr_initializer(const std::vector<const expr*>&);
    void initialize(const goal_lineage*) override;
private:
    i_goal_expr_store& ges;
    std::vector<const expr*> initial_exprs;
};

#endif
