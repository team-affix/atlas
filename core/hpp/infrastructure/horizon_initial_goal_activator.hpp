#ifndef HORIZON_INITIAL_GOAL_ACTIVATOR_HPP
#define HORIZON_INITIAL_GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IInitialGoalActivator, typename IMakeInitialGoalLineage,
         typename ISetGoalWeight, typename IGetInitialGoalWeight>
struct horizon_initial_goal_activator {
    horizon_initial_goal_activator(IInitialGoalActivator&, IMakeInitialGoalLineage&,
                                   ISetGoalWeight&, IGetInitialGoalWeight&);
    void activate_initial_goal(subgoal_id idx);
private:
    IInitialGoalActivator& initial_goal_activator_;
    IMakeInitialGoalLineage& make_initial_goal_lineage_;
    ISetGoalWeight& goal_weights_;
    IGetInitialGoalWeight& initial_goal_weight_;
};

template<typename IIGA, typename IMIGL, typename ISGW, typename IIGW>
horizon_initial_goal_activator<IIGA,IMIGL,ISGW,IIGW>::horizon_initial_goal_activator(
    IIGA& iga, IMIGL& migl, ISGW& gw, IIGW& igw)
    : initial_goal_activator_(iga), make_initial_goal_lineage_(migl),
      goal_weights_(gw), initial_goal_weight_(igw) {}

template<typename IIGA, typename IMIGL, typename ISGW, typename IIGW>
void horizon_initial_goal_activator<IIGA,IMIGL,ISGW,IIGW>::activate_initial_goal(subgoal_id idx) {
    initial_goal_activator_.activate_initial_goal(idx);
    const goal_lineage* gl = make_initial_goal_lineage_.make(idx);
    goal_weights_.set(gl, initial_goal_weight_.get());
}

#endif
