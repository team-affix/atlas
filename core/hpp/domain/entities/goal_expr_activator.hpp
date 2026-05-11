#ifndef GOAL_EXPR_ACTIVATOR_HPP
#define GOAL_EXPR_ACTIVATOR_HPP

#include <vector>
#include "../interfaces/i_goal_expr_activator.hpp"
#include "../interfaces/i_applicant_frontier.hpp"
#include "../value_objects/expr.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_expr_frontier.hpp"
#include "../interfaces/i_copier.hpp"

struct goal_expr_activator : i_goal_expr_activator {
    explicit goal_expr_activator(const std::vector<const expr*>& initial_exprs);
    void start_resolution(const resolution_lineage*) override;
    void activate(const goal_lineage*) override;
private:
    i_database& db;
    i_expr_frontier& ef;
    i_applicant_frontier& af;
    i_copier& cp;
    std::vector<const expr*> initial_exprs;
    std::vector<const expr*> translated_rule_body;
};

#endif
