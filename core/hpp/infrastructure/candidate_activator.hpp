#ifndef CANDIDATE_ACTIVATOR_HPP
#define CANDIDATE_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/framed_expr.hpp"

template<typename IFrameAllocator, typename ICandidateFrameOffsets,
         typename IMhuEliminationGenerator, typename IEliminationBacklog,
         typename IGoalExprs, typename IDb, typename IGoalCandidateRules>
struct candidate_activator {
    candidate_activator(IFrameAllocator&, ICandidateFrameOffsets&,
                        IMhuEliminationGenerator&, IEliminationBacklog&,
                        IGoalExprs&, IDb&, IGoalCandidateRules&);
    void activate(const resolution_lineage*);
private:
    IFrameAllocator& frame_allocator;
    ICandidateFrameOffsets& set_candidate_frame_offset;
    IMhuEliminationGenerator& try_add_mhu_head;
    IEliminationBacklog& is_backlogged_elimination;
    IGoalExprs& get_goal_expr;
    IDb& get_rule;
    IGoalCandidateRules& link_goal_candidate;
};

template<typename IFA, typename ICFO, typename IMEG, typename IEB,
         typename IGE, typename IDB, typename IGCR>
candidate_activator<IFA, ICFO, IMEG, IEB, IGE, IDB, IGCR>::candidate_activator(
    IFA& fa, ICFO& cfo, IMEG& meg, IEB& eb, IGE& ge, IDB& db, IGCR& gcr)
    : frame_allocator(fa), set_candidate_frame_offset(cfo), try_add_mhu_head(meg),
      is_backlogged_elimination(eb), get_goal_expr(ge), get_rule(db),
      link_goal_candidate(gcr) {}

template<typename IFA, typename ICFO, typename IMEG, typename IEB,
         typename IGE, typename IDB, typename IGCR>
void candidate_activator<IFA, ICFO, IMEG, IEB, IGE, IDB, IGCR>::activate(
    const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;
    const rule* r = get_rule.get(rl->idx);
    if (is_backlogged_elimination.is_backlogged_elimination(rl)) return;
    const uint32_t frame_offset = frame_allocator.bump(r->var_count);
    const framed_expr goal = get_goal_expr.get(gl);
    const framed_expr head{r->head, frame_offset};
    if (!try_add_mhu_head.try_add_head(rl, goal, head)) return;
    set_candidate_frame_offset.set(rl, frame_offset);
    link_goal_candidate.link_goal_candidate(gl, rl->idx);
}

#endif
