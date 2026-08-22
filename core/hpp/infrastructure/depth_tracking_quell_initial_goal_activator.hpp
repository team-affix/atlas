#ifndef DEPTH_TRACKING_QUELL_INITIAL_GOAL_ACTIVATOR_HPP
#define DEPTH_TRACKING_QUELL_INITIAL_GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IActivateQuellInitialGoal, typename IMakeInitialGoalLineage,
         typename ISetGoalDepth>
struct depth_tracking_quell_initial_goal_activator {
    depth_tracking_quell_initial_goal_activator(IActivateQuellInitialGoal&,
                                                IMakeInitialGoalLineage&, ISetGoalDepth&);
    void activate_initial_goal(subgoal_id idx);
private:
    IActivateQuellInitialGoal& activate_quell_initial_goal_;
    IMakeInitialGoalLineage& make_initial_goal_lineage_;
    ISetGoalDepth& set_goal_depth_;
};

template<typename IAQIG, typename IMIGL, typename ISGD>
depth_tracking_quell_initial_goal_activator<IAQIG, IMIGL, ISGD>::
depth_tracking_quell_initial_goal_activator(IAQIG& aqig, IMIGL& migl, ISGD& sgd)
    : activate_quell_initial_goal_(aqig)
    , make_initial_goal_lineage_(migl)
    , set_goal_depth_(sgd) {}

template<typename IAQIG, typename IMIGL, typename ISGD>
void depth_tracking_quell_initial_goal_activator<IAQIG, IMIGL, ISGD>::
activate_initial_goal(subgoal_id idx) {
    // Only an index arrives here, so the lineage has to be resolved before the
    // depth can be recorded against it. The depth must land in the store before
    // delegating: the work credited downstream is looked up against it.
    const goal_lineage* gl = make_initial_goal_lineage_.make(idx);
    set_goal_depth_.set(gl, 0);
    activate_quell_initial_goal_.activate_initial_goal(idx);
}

#endif
