#ifndef GOAL_CANDIDATE_ACTIVATOR_VISITOR_HPP
#define GOAL_CANDIDATE_ACTIVATOR_VISITOR_HPP

#include "../interfaces/i_goal_candidate_activator_visitor.hpp"
#include "../interfaces/i_copier.hpp"
#include "../interfaces/i_set_candidate_translation_map.hpp"
#include "../interfaces/i_mhu_elimination_generator.hpp"
#include "../interfaces/i_candidate_activator.hpp"

struct goal_candidate_activator_visitor : i_goal_candidate_activator_visitor {
    goal_candidate_activator_visitor(
        i_copier& copier,
        i_set_candidate_translation_map& set_candidate_translation_map,
        i_mhu_elimination_generator& mhu_elimination_generator,
        i_candidate_activator& candidate_activator);
    void visit(const rule*) override;
private:
    i_copier& copier;
    i_set_candidate_translation_map& set_candidate_translation_map;
    i_mhu_elimination_generator& mhu_elimination_generator;
    i_candidate_activator& candidate_activator;
};

#endif
