#ifndef GOAL_ACTIVATOR_HPP
#define GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/rule.hpp"

template<typename ISetGoalExpr, typename IInsertGoalCandidates, typename IInsertActiveGoal,
         typename IGetCandidateFrameOffset, typename IGetResolutionRule>
struct goal_activator {
    goal_activator(ISetGoalExpr&, IInsertGoalCandidates&, IInsertActiveGoal&,
                   IGetCandidateFrameOffset&, IGetResolutionRule&);
    void activate(const goal_lineage*);
private:
    ISetGoalExpr& set_goal_expr_;
    IInsertGoalCandidates& insert_goal_candidates_;
    IInsertActiveGoal& insert_active_goal_;
    IGetCandidateFrameOffset& get_candidate_frame_offset_;
    IGetResolutionRule& get_resolution_rule_;
};

template<typename ISGE, typename IIGC, typename IIAG, typename IGCFO, typename IGRR>
goal_activator<ISGE, IIGC, IIAG, IGCFO, IGRR>::goal_activator(
    ISGE& ge, IIGC& gcr, IIAG& ag, IGCFO& cfo, IGRR& grr)
    : set_goal_expr_(ge), insert_goal_candidates_(gcr), insert_active_goal_(ag), get_candidate_frame_offset_(cfo), get_resolution_rule_(grr) {}

template<typename ISGE, typename IIGC, typename IIAG, typename IGCFO, typename IGRR>
void goal_activator<ISGE, IIGC, IIAG, IGCFO, IGRR>::activate(const goal_lineage* gl) {
    const resolution_lineage* rl = gl->parent;
    const rule* r = get_resolution_rule_.get(rl);
    const uint32_t frame_offset = get_candidate_frame_offset_.get(rl);
    const expr* body_expr = r->body.at(gl->idx);
    set_goal_expr_.set(gl, framed_expr{body_expr, frame_offset});
    insert_goal_candidates_.insert(gl);
    insert_active_goal_.insert_active_goal(gl);
}

#endif
