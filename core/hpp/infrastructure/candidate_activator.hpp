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
    IBumpFrameAllocator& frame_allocator_;
    ISetCandidateFrameOffset& set_candidate_frame_offset_;
    ITryAddMhuHead& try_add_mhu_head_;
    IIsBackloggedElimination& is_backlogged_elimination_;
    IGetGoalExpr& get_goal_expr_;
    IGetRule& get_rule_;
    ILinkGoalCandidate& link_goal_candidate_;
};

template<typename IBFA, typename ISCFO, typename ITAMH, typename IIBE,
         typename IGGE, typename IGR, typename ILGC>
candidate_activator<IBFA, ISCFO, ITAMH, IIBE, IGGE, IGR, ILGC>::candidate_activator(
    IBFA& fa, ISCFO& cfo, ITAMH& meg, IIBE& eb, IGGE& ge, IGR& db, ILGC& gcr)
    : frame_allocator_(fa), set_candidate_frame_offset_(cfo), try_add_mhu_head_(meg), is_backlogged_elimination_(eb), get_goal_expr_(ge), get_rule_(db), link_goal_candidate_(gcr) {}

template<typename IBFA, typename ISCFO, typename ITAMH, typename IIBE,
         typename IGGE, typename IGR, typename ILGC>
void candidate_activator<IBFA, ISCFO, ITAMH, IIBE, IGGE, IGR, ILGC>::activate(
    const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;
    const rule* r = get_rule_.get_rule(rl->idx);
    if (is_backlogged_elimination_.is_backlogged_elimination(rl)) return;
    const uint32_t frame_offset = frame_allocator_.bump(r->var_count);
    const framed_expr goal = get_goal_expr_.get(gl);
    const framed_expr head{r->head, frame_offset};
    if (!try_add_mhu_head_.try_add_head(rl, goal, head)) return;
    set_candidate_frame_offset_.set(rl, frame_offset);
    link_goal_candidate_.link_goal_candidate(gl, rl->idx);
}

#endif
