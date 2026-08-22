#ifndef DEPTH_TRACKING_QUELL_GOAL_ACTIVATOR_HPP
#define DEPTH_TRACKING_QUELL_GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IActivateQuellGoal, typename IGetGoalDepth, typename ISetGoalDepth>
struct depth_tracking_quell_goal_activator {
    depth_tracking_quell_goal_activator(IActivateQuellGoal&, IGetGoalDepth&, ISetGoalDepth&);
    void activate(const goal_lineage*);
private:
    IActivateQuellGoal& activate_quell_goal_;
    IGetGoalDepth& get_goal_depth_;
    ISetGoalDepth& set_goal_depth_;
};

template<typename IAQG, typename IGGD, typename ISGD>
depth_tracking_quell_goal_activator<IAQG, IGGD, ISGD>::depth_tracking_quell_goal_activator(
    IAQG& aqg, IGGD& ggd, ISGD& sgd)
    : activate_quell_goal_(aqg)
    , get_goal_depth_(ggd)
    , set_goal_depth_(sgd) {}

template<typename IAQG, typename IGGD, typename ISGD>
void depth_tracking_quell_goal_activator<IAQG, IGGD, ISGD>::activate(const goal_lineage* gl) {
    // The depth must land in the store before delegating: the work credited
    // downstream is looked up against whatever depth this goal has by then.
    const goal_lineage* parent = gl->parent->parent;
    set_goal_depth_.set(gl, get_goal_depth_.get(parent) + 1);
    activate_quell_goal_.activate(gl);
}

#endif
