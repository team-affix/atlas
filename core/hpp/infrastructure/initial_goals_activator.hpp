#ifndef INITIAL_GOALS_ACTIVATOR_HPP
#define INITIAL_GOALS_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IGetInitialGoalCount, typename IInitialGoalActivator,
         typename IMakeInitialGoalLineage, typename IActivateGoalCandidates>
struct initial_goals_activator {
    initial_goals_activator(IGetInitialGoalCount&, IInitialGoalActivator&,
                            IMakeInitialGoalLineage&, IActivateGoalCandidates&);
    bool activate_initial_goals_and_candidates();
private:
    IGetInitialGoalCount& get_initial_goal_count_;
    IInitialGoalActivator& activate_initial_goal_;
    IMakeInitialGoalLineage& make_initial_goal_lineage_;
    IActivateGoalCandidates& activate_goal_candidates_;
};

template<typename IIGC, typename IIGA, typename IMIGL, typename IGCA>
initial_goals_activator<IIGC, IIGA, IMIGL, IGCA>::initial_goals_activator(
    IIGC& ige, IIGA& iga, IMIGL& migl, IGCA& gca)
    : get_initial_goal_count_(ige), activate_initial_goal_(iga),
      make_initial_goal_lineage_(migl), activate_goal_candidates_(gca) {}

template<typename IIGC, typename IIGA, typename IMIGL, typename IGCA>
bool initial_goals_activator<IIGC, IIGA, IMIGL, IGCA>::activate_initial_goals_and_candidates() {
    for (size_t i = 0; i < get_initial_goal_count_.count(); ++i) {
        activate_initial_goal_.activate_initial_goal(i);
        const goal_lineage* gl = make_initial_goal_lineage_.make(i);
        if (!activate_goal_candidates_.activate_goal_candidates(gl))
            return false;
    }
    return true;
}

#endif
