#ifndef GET_UNIT_RESOLUTION_HPP
#define GET_UNIT_RESOLUTION_HPP

#include "value_objects/lineage.hpp"

template<typename IGetGoalCandidateRuleIds, typename ILineagePool>
struct get_unit_resolution {
    get_unit_resolution(IGetGoalCandidateRuleIds& gcr, ILineagePool& lp);
    const resolution_lineage* get(const goal_lineage*);
private:
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids;
    ILineagePool& make_resolution_lineage;
};

template<typename IGCR, typename ILP>
get_unit_resolution<IGCR, ILP>::get_unit_resolution(IGCR& gcr, ILP& lp)
    : get_goal_candidate_rule_ids(gcr), make_resolution_lineage(lp) {}

template<typename IGCR, typename ILP>
const resolution_lineage* get_unit_resolution<IGCR, ILP>::get(const goal_lineage* gl) {
    return make_resolution_lineage.make_resolution_lineage(
        gl, get_goal_candidate_rule_ids.get(gl).front());
}

#endif
