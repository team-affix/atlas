#ifndef ELIMINATION_ROUTER_HPP
#define ELIMINATION_ROUTER_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/elimination_result.hpp"

template<typename IGetGoalCandidateRuleIds, typename IIsActiveGoal,
         typename IInsertBackloggedElimination, typename ICandidateDeactivator>
struct elimination_router {
    elimination_router(IGetGoalCandidateRuleIds&, IIsActiveGoal&,
                       IInsertBackloggedElimination&, ICandidateDeactivator&);
    elimination_result route(const resolution_lineage*);
private:
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids;
    IIsActiveGoal& is_active_goal;
    IInsertBackloggedElimination& insert_backlogged_elimination;
    ICandidateDeactivator& candidate_deactivator;
};

template<typename IGCRI, typename IIAG, typename IIBE, typename ICD>
elimination_router<IGCRI, IIAG, IIBE, ICD>::elimination_router(
    IGCRI& gcr, IIAG& ag, IIBE& eb, ICD& cd)
    : get_goal_candidate_rule_ids(gcr), is_active_goal(ag),
      insert_backlogged_elimination(eb), candidate_deactivator(cd) {}

template<typename IGCRI, typename IIAG, typename IIBE, typename ICD>
elimination_result elimination_router<IGCRI, IIAG, IIBE, ICD>::route(
    const resolution_lineage* rl) {
    if (!is_active_goal.is_active_goal(rl->parent)) {
        insert_backlogged_elimination.insert_backlogged_elimination(rl);
        return elimination_result::added_to_backlog;
    }
    if (!get_goal_candidate_rule_ids.get(rl->parent).contains(rl->idx))
        return elimination_result::already_deactivated;
    candidate_deactivator.deactivate(rl);
    return elimination_result::eliminated;
}

#endif
