#ifndef CANDIDATE_ACTIVATOR_HPP
#define CANDIDATE_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/framed_expr.hpp"

template<typename IBumpFrameAllocator, typename ISetCandidateFrameOffset,
         typename ITryAddMhuHead, typename IIsBackloggedElimination,
         typename IGetGoalExpr, typename IGetRule, typename ILinkGoalCandidate>
struct candidate_activator {
    candidate_activator(IBumpFrameAllocator&, ISetCandidateFrameOffset&,
                        ITryAddMhuHead&, IIsBackloggedElimination&,
                        IGetGoalExpr&, IGetRule&, ILinkGoalCandidate&);
    void activate(const resolution_lineage*);
private:
    IBumpFrameAllocator& frame_allocator;
    ISetCandidateFrameOffset& set_candidate_frame_offset;
    ITryAddMhuHead& try_add_mhu_head;
    IIsBackloggedElimination& is_backlogged_elimination;
    IGetGoalExpr& get_goal_expr;
    IGetRule& get_rule;
    ILinkGoalCandidate& link_goal_candidate;
};

template<typename IBFA, typename ISCFO, typename ITAMH, typename IIBE,
         typename IGGE, typename IGR, typename ILGC>
candidate_activator<IBFA, ISCFO, ITAMH, IIBE, IGGE, IGR, ILGC>::candidate_activator(
    IBFA& fa, ISCFO& cfo, ITAMH& meg, IIBE& eb, IGGE& ge, IGR& db, ILGC& gcr)
    : frame_allocator(fa), set_candidate_frame_offset(cfo), try_add_mhu_head(meg),
      is_backlogged_elimination(eb), get_goal_expr(ge), get_rule(db),
      link_goal_candidate(gcr) {}

template<typename IBFA, typename ISCFO, typename ITAMH, typename IIBE,
         typename IGGE, typename IGR, typename ILGC>
void candidate_activator<IBFA, ISCFO, ITAMH, IIBE, IGGE, IGR, ILGC>::activate(
    const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;
    const rule* r = get_rule.get_rule(rl->idx);
    if (is_backlogged_elimination.is_backlogged_elimination(rl)) return;
    const uint32_t frame_offset = frame_allocator.bump(r->var_count);
    const framed_expr goal = get_goal_expr.get(gl);
    const framed_expr head{r->head, frame_offset};
    if (!try_add_mhu_head.try_add_head(rl, goal, head)) return;
    set_candidate_frame_offset.set(rl, frame_offset);
    link_goal_candidate.link_goal_candidate(gl, rl->idx);
}

#endif
