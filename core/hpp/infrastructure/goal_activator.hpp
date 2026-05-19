#ifndef GOAL_ACTIVATOR_HPP
#define GOAL_ACTIVATOR_HPP

#include "../interfaces/i_goal_activator.hpp"
#include "../interfaces/i_activate_goal_expr.hpp"
#include "../interfaces/i_get_candidate_translation_map.hpp"
#include "../interfaces/i_copier.hpp"

struct goal_activator : i_goal_activator {
    goal_activator(
        i_activate_goal_expr& age,
        i_get_candidate_translation_map& gctm,
        i_copier& copier);
    void activate(const goal_lineage*) override;
private:
    i_activate_goal_expr& age;
    i_get_candidate_translation_map& gctm;
    i_copier& copier;
};

#endif
