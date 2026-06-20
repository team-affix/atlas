#ifndef INITIAL_GOALS_ACTIVATOR_HPP
#define INITIAL_GOALS_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IInitialGoalExprs, typename IInitialGoalActivator,
         typename IMakeInitialGoalLineage, typename IGoalCandidatesActivator>
struct initial_goals_activator {
    initial_goals_activator(IInitialGoalExprs&, IInitialGoalActivator&,
                            IMakeInitialGoalLineage&, IGoalCandidatesActivator&);
    bool activate_initial_goals_and_candidates();
private:
    IInitialGoalExprs& get_initial_goal_count;
    IInitialGoalActivator& activate_initial_goal;
    IMakeInitialGoalLineage& make_initial_goal_lineage;
    IGoalCandidatesActivator& activate_goal_candidates;
};

template<typename IIGE, typename IIGA, typename IMIGL, typename IGCA>
initial_goals_activator<IIGE, IIGA, IMIGL, IGCA>::initial_goals_activator(
    IIGE& ige, IIGA& iga, IMIGL& migl, IGCA& gca)
    : get_initial_goal_count(ige), activate_initial_goal(iga),
      make_initial_goal_lineage(migl), activate_goal_candidates(gca) {}

template<typename IIGE, typename IIGA, typename IMIGL, typename IGCA>
bool initial_goals_activator<IIGE, IIGA, IMIGL, IGCA>::activate_initial_goals_and_candidates() {
    for (size_t i = 0; i < get_initial_goal_count.count(); ++i) {
        activate_initial_goal.activate_initial_goal(i);
        const goal_lineage* gl = make_initial_goal_lineage.make(i);
        if (!activate_goal_candidates.activate_goal_candidates(gl))
            return false;
    }
    return true;
}

#endif
