#ifndef SUBGOALS_ACTIVATOR_HPP
#define SUBGOALS_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IMakeGoalLineage, typename IGoalActivator,
         typename IGetRule, typename IActivateGoalCandidates>
struct subgoals_activator {
    subgoals_activator(IMakeGoalLineage&, IGoalActivator&, IGetRule&, IActivateGoalCandidates&);
    bool activate_subgoals_and_candidates(const resolution_lineage*);
private:
    IMakeGoalLineage& make_goal_lineage;
    IGoalActivator& goal_activator;
    IGetRule& get_rule;
    IActivateGoalCandidates& activate_goal_candidates;
};

template<typename IMGL, typename IGA, typename IGR, typename IGCA>
subgoals_activator<IMGL, IGA, IGR, IGCA>::subgoals_activator(
    IMGL& lp, IGA& ga, IGR& db, IGCA& gca)
    : make_goal_lineage(lp), goal_activator(ga), get_rule(db),
      activate_goal_candidates(gca) {}

template<typename IMGL, typename IGA, typename IGR, typename IGCA>
bool subgoals_activator<IMGL, IGA, IGR, IGCA>::activate_subgoals_and_candidates(
    const resolution_lineage* rl) {
    const rule* rule = get_rule.get_rule(rl->idx);
    for (size_t body_idx = 0; body_idx < rule->body.size(); ++body_idx) {
        const goal_lineage* gl = make_goal_lineage.make_goal_lineage(rl, body_idx);
        goal_activator.activate(gl);
        if (!activate_goal_candidates.activate_goal_candidates(gl))
            return false;
    }
    return true;
}

#endif
