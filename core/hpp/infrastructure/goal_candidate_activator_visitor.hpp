#ifndef GOAL_CANDIDATE_ACTIVATOR_VISITOR_HPP
#define GOAL_CANDIDATE_ACTIVATOR_VISITOR_HPP

#include "../interfaces/i_goal_candidate_activator_visitor.hpp"
#include "../value_objects/lineage.hpp"
#include "../interfaces/i_bind_map.hpp"
#include "../interfaces/i_bind_map_factory.hpp"
#include "../interfaces/i_overlay_bind_map_factory.hpp"
#include "../interfaces/i_unifier_factory.hpp"
#include "../interfaces/i_copier.hpp"
#include "../interfaces/i_set_candidate_translation_map.hpp"
#include "../interfaces/i_mhu_elimination_generator.hpp"
#include "../interfaces/i_candidate_activator.hpp"
#include "../interfaces/i_elimination_backlog.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_get_goal_expr.hpp"

struct goal_candidate_activator_visitor : i_goal_candidate_activator_visitor {
    goal_candidate_activator_visitor(
        const goal_lineage* gl,
        i_copier& copier,
        i_bind_map& common_bind_map,
        i_bind_map_factory& bind_map_factory,
        i_overlay_bind_map_factory& overlay_bind_map_factory,
        i_unifier_factory& unifier_factory,
        i_set_candidate_translation_map& set_candidate_translation_map,
        i_mhu_elimination_generator& mhu_elimination_generator,
        i_candidate_activator& candidate_activator,
        i_elimination_backlog& elimination_backlog,
        i_lineage_pool& lp,
        i_get_goal_expr& gge);
    void visit(const rule*) override;
private:
    const goal_lineage* gl;
    i_copier& copier;
    i_bind_map& common_bind_map;
    i_bind_map_factory& bind_map_factory;
    i_overlay_bind_map_factory& overlay_bind_map_factory;
    i_unifier_factory& unifier_factory;
    i_set_candidate_translation_map& set_candidate_translation_map;
    i_mhu_elimination_generator& mhu_elimination_generator;
    i_candidate_activator& candidate_activator;
    i_elimination_backlog& elimination_backlog;
    i_lineage_pool& lp;
    i_get_goal_expr& gge;
};

#endif
