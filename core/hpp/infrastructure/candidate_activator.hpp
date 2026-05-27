#ifndef CANDIDATE_ACTIVATOR_HPP
#define CANDIDATE_ACTIVATOR_HPP

#include "../interfaces/i_candidate_activator.hpp"
#include "../interfaces/i_copier.hpp"
#include "../interfaces/i_set_candidate_translation_map.hpp"
#include "../interfaces/i_mhu_elimination_generator.hpp"
#include "../interfaces/i_is_backlogged_elimination.hpp"
#include "../interfaces/i_get_goal_expr.hpp"
#include "../interfaces/i_link_goal_candidate.hpp"

struct candidate_activator : i_candidate_activator {
    candidate_activator(
        i_copier& copier,
        i_set_candidate_translation_map& set_candidate_translation_map,
        i_mhu_elimination_generator& mhu_elimination_generator,
        i_is_backlogged_elimination& is_backlogged_elimination,
        i_get_goal_expr& get_goal_expr,
        i_link_goal_candidate& link_goal_candidate);
    void activate(const resolution_lineage*) override;
private:
    i_copier& copier;
    i_set_candidate_translation_map& set_candidate_translation_map;
    i_mhu_elimination_generator& mhu_elimination_generator;
    i_is_backlogged_elimination& is_backlogged_elimination;
    i_get_goal_expr& get_goal_expr;
    i_link_goal_candidate& link_goal_candidate;
};

#endif
