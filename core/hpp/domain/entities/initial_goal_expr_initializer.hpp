#ifndef INITIAL_GOAL_EXPR_INITIALIZER_HPP
#define INITIAL_GOAL_EXPR_INITIALIZER_HPP

#include "../interfaces/i_initial_goal_expr_initializer.hpp"

struct initial_goal_expr_initializer : i_initial_goal_expr_initializer {
    initial_goal_expr_initializer();
    void initialize(const goal_lineage*) override;
};

#endif
