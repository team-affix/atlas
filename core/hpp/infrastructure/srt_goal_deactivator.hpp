#ifndef SRT_GOAL_DEACTIVATOR_HPP
#define SRT_GOAL_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IUnsetGoalExpr, typename IEraseGoalCandidates>
struct srt_goal_deactivator {
    srt_goal_deactivator(IUnsetGoalExpr& ge, IEraseGoalCandidates& gcr);
    void deactivate(const goal_lineage*);
private:
    IUnsetGoalExpr& unset_goal_expr;
    IEraseGoalCandidates& erase_goal_candidates;
};

template<typename IUGE, typename IEGC>
srt_goal_deactivator<IUGE, IEGC>::srt_goal_deactivator(IUGE& ge, IEGC& gcr)
    : unset_goal_expr(ge), erase_goal_candidates(gcr) {}

template<typename IUGE, typename IEGC>
void srt_goal_deactivator<IUGE, IEGC>::deactivate(const goal_lineage* gl) {
    erase_goal_candidates.erase(gl);
    unset_goal_expr.unset(gl);
}

#endif
