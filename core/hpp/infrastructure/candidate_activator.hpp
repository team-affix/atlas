#ifndef CANDIDATE_ACTIVATOR_HPP
#define CANDIDATE_ACTIVATOR_HPP

#include "../interfaces/i_candidate_activator.hpp"
#include "../interfaces/i_copier.hpp"
#include "../interfaces/i_activate_candidate_translation_map.hpp"
#include "../interfaces/i_mhu_elimination_generator.hpp"
#include "../interfaces/i_elimination_backlog.hpp"
#include "../interfaces/i_get_goal_expr.hpp"

struct candidate_activator : i_candidate_activator {
    candidate_activator(
        i_copier& copier,
        i_activate_candidate_translation_map& actm,
        i_mhu_elimination_generator& mhu_elimination_generator,
        i_elimination_backlog& elimination_backlog,
        i_get_goal_expr& gge);
    void activate(const resolution_lineage*) override;
private:
    i_copier& copier;
    i_activate_candidate_translation_map& actm;
    i_mhu_elimination_generator& mhu_elimination_generator;
    i_elimination_backlog& elimination_backlog;
    i_get_goal_expr& gge;
};

#endif
