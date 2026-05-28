#ifndef GOAL_ACTIVATOR_HPP
#define GOAL_ACTIVATOR_HPP

#include "../interfaces/i_goal_activator.hpp"
#include "../interfaces/i_set_goal_expr.hpp"
#include "../interfaces/i_get_candidate_translation_map.hpp"
#include "../interfaces/i_get_resolution_rule.hpp"
#include "../interfaces/i_copier.hpp"

struct goal_activator : i_goal_activator {
    goal_activator(
        i_set_goal_expr& set_goal_expr,
        i_get_candidate_translation_map& get_candidate_translation_map,
        i_get_resolution_rule& get_resolution_rule,
        i_copier& copier);
    void activate(const goal_lineage*) override;
private:
    i_set_goal_expr& set_goal_expr;
    i_get_candidate_translation_map& get_candidate_translation_map;
    i_get_resolution_rule& get_resolution_rule;
    i_copier& copier;
};

#endif
