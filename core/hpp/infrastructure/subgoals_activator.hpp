#ifndef SUBGOALS_ACTIVATOR_HPP
#define SUBGOALS_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename ILineagePool, typename IGoalActivator,
         typename IDb, typename IGoalCandidatesActivator>
struct subgoals_activator {
    subgoals_activator(ILineagePool&, IGoalActivator&, IDb&, IGoalCandidatesActivator&);
    bool activate_subgoals_and_candidates(const resolution_lineage*);
private:
    ILineagePool& make_goal_lineage;
    IGoalActivator& goal_activator;
    IDb& get_rule;
    IGoalCandidatesActivator& activate_goal_candidates;
};

template<typename ILP, typename IGA, typename IDB, typename IGCA>
subgoals_activator<ILP, IGA, IDB, IGCA>::subgoals_activator(
    ILP& lp, IGA& ga, IDB& db, IGCA& gca)
    : make_goal_lineage(lp), goal_activator(ga), get_rule(db),
      activate_goal_candidates(gca) {}

template<typename ILP, typename IGA, typename IDB, typename IGCA>
bool subgoals_activator<ILP, IGA, IDB, IGCA>::activate_subgoals_and_candidates(
    const resolution_lineage* rl) {
    const rule* rule = get_rule.get(rl->idx);
    for (size_t body_idx = 0; body_idx < rule->body.size(); ++body_idx) {
        const goal_lineage* gl = make_goal_lineage.make_goal_lineage(rl, body_idx);
        goal_activator.activate(gl);
        if (!activate_goal_candidates.activate_goal_candidates(gl))
            return false;
    }
    return true;
}

#endif
