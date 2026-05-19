#ifndef GOAL_CANDIDATE_DEACTIVATOR_VISITOR_HPP
#define GOAL_CANDIDATE_DEACTIVATOR_VISITOR_HPP

#include "../interfaces/i_goal_candidate_deactivator_visitor.hpp"
#include "../value_objects/lineage.hpp"
#include "../interfaces/i_mhu_elimination_generator.hpp"
#include "../interfaces/i_candidate_deactivator.hpp"
#include "../interfaces/i_elimination_backlog.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_deactivate_candidate_translation_map.hpp"

struct goal_candidate_deactivator_visitor : i_goal_candidate_deactivator_visitor {
    goal_candidate_deactivator_visitor(
        const goal_lineage* gl,
        i_mhu_elimination_generator& mhu_elimination_generator,
        i_candidate_deactivator& candidate_deactivator,
        i_elimination_backlog& elimination_backlog,
        i_lineage_pool& lp,
        i_deactivate_candidate_translation_map& dctm);
    void visit(const rule*) override;
private:
    const goal_lineage* gl;
    i_mhu_elimination_generator& mhu_elimination_generator;
    i_candidate_deactivator& candidate_deactivator;
    i_elimination_backlog& elimination_backlog;
    i_lineage_pool& lp;
    i_deactivate_candidate_translation_map& dctm;
};

#endif
