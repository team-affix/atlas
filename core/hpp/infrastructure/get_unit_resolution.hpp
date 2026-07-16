#ifndef GET_UNIT_RESOLUTION_HPP
#define GET_UNIT_RESOLUTION_HPP

#include "value_objects/lineage.hpp"

template<typename IGetGoalCandidateRuleIds, typename IMakeResolutionLineage>
struct get_unit_resolution {
    get_unit_resolution(IGetGoalCandidateRuleIds& gcr, IMakeResolutionLineage& lp);
    const resolution_lineage* get(const goal_lineage*);
private:
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;
    IMakeResolutionLineage& make_resolution_lineage_;
};

template<typename IGCR, typename IMRL>
get_unit_resolution<IGCR, IMRL>::get_unit_resolution(IGCR& gcr, IMRL& lp)
    : get_goal_candidate_rule_ids_(gcr), make_resolution_lineage_(lp) {}

template<typename IGCR, typename IMRL>
const resolution_lineage* get_unit_resolution<IGCR, IMRL>::get(const goal_lineage* gl) {
    return make_resolution_lineage_.make_resolution_lineage(
        gl, get_goal_candidate_rule_ids_.get(gl).front());
}

#endif
