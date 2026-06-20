#ifndef GOAL_ACTIVATOR_HPP
#define GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/rule.hpp"

template<typename IGoalExprs, typename IGoalCandidateRules, typename IActiveGoals,
         typename ICandidateFrameOffsets, typename IGetResolutionRule>
struct goal_activator {
    goal_activator(IGoalExprs&, IGoalCandidateRules&, IActiveGoals&,
                   ICandidateFrameOffsets&, IGetResolutionRule&);
    void activate(const goal_lineage*);
private:
    IGoalExprs& set_goal_expr;
    IGoalCandidateRules& insert_goal_candidates;
    IActiveGoals& insert_active_goal;
    ICandidateFrameOffsets& get_candidate_frame_offset;
    IGetResolutionRule& get_resolution_rule;
};

template<typename IGE, typename IGCR, typename IAG, typename ICFO, typename IGRR>
goal_activator<IGE, IGCR, IAG, ICFO, IGRR>::goal_activator(
    IGE& ge, IGCR& gcr, IAG& ag, ICFO& cfo, IGRR& grr)
    : set_goal_expr(ge), insert_goal_candidates(gcr), insert_active_goal(ag),
      get_candidate_frame_offset(cfo), get_resolution_rule(grr) {}

template<typename IGE, typename IGCR, typename IAG, typename ICFO, typename IGRR>
void goal_activator<IGE, IGCR, IAG, ICFO, IGRR>::activate(const goal_lineage* gl) {
    const resolution_lineage* rl = gl->parent;
    const rule* r = get_resolution_rule.get(rl);
    const uint32_t frame_offset = get_candidate_frame_offset.get(rl);
    const expr* body_expr = r->body.at(gl->idx);
    set_goal_expr.set(gl, framed_expr{body_expr, frame_offset});
    insert_goal_candidates.insert(gl);
    insert_active_goal.insert_active_goal(gl);
}

#endif
