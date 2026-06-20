#ifndef SRT_GOAL_DEACTIVATOR_HPP
#define SRT_GOAL_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IGoalExprs, typename IGoalCandidateRules>
struct srt_goal_deactivator {
    srt_goal_deactivator(IGoalExprs& ge, IGoalCandidateRules& gcr);
    void deactivate(const goal_lineage*);
private:
    IGoalExprs& unset_goal_expr;
    IGoalCandidateRules& erase_goal_candidates;
};

template<typename IGE, typename IGCR>
srt_goal_deactivator<IGE, IGCR>::srt_goal_deactivator(IGE& ge, IGCR& gcr)
    : unset_goal_expr(ge), erase_goal_candidates(gcr) {}

template<typename IGE, typename IGCR>
void srt_goal_deactivator<IGE, IGCR>::deactivate(const goal_lineage* gl) {
    erase_goal_candidates.erase(gl);
    unset_goal_expr.unset(gl);
}

#endif
