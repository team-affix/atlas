#ifndef QUELL_GOAL_ACTIVATOR_HPP
#define QUELL_GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IGoalActivator, typename IGetGoalDepth, typename ISetGoalDepth,
         typename ISetGoalWorkValue, typename IGetGoalWork, typename IAddRemainingWork>
struct quell_goal_activator {
    quell_goal_activator(IGoalActivator&, IGetGoalDepth&, ISetGoalDepth&,
                         ISetGoalWorkValue&, IGetGoalWork&, IAddRemainingWork&);
    void activate(const goal_lineage*);
private:
    IGoalActivator& goal_activator_;
    IGetGoalDepth& get_goal_depth_;
    ISetGoalDepth& set_goal_depth_;
    ISetGoalWorkValue& set_goal_work_value_;
    IGetGoalWork& get_goal_work_;
    IAddRemainingWork& add_remaining_work_;
};

template<typename IGA, typename IGGD, typename ISGD, typename ISGWV, typename IGGW,
         typename IARW>
quell_goal_activator<IGA, IGGD, ISGD, ISGWV, IGGW, IARW>::quell_goal_activator(
    IGA& ga, IGGD& ggd, ISGD& sgd, ISGWV& sgwv, IGGW& ggw, IARW& arw)
    : goal_activator_(ga)
    , get_goal_depth_(ggd)
    , set_goal_depth_(sgd)
    , set_goal_work_value_(sgwv)
    , get_goal_work_(ggw)
    , add_remaining_work_(arw) {}

template<typename IGA, typename IGGD, typename ISGD, typename ISGWV, typename IGGW,
         typename IARW>
void quell_goal_activator<IGA, IGGD, ISGD, ISGWV, IGGW, IARW>::activate(
    const goal_lineage* gl) {
    goal_activator_.activate(gl);
    const goal_lineage* parent = gl->parent->parent;
    const size_t depth = get_goal_depth_.get(parent) + 1;
    const double work = get_goal_work_.get(depth);
    set_goal_depth_.set(gl, depth);
    set_goal_work_value_.set(gl, work);
    add_remaining_work_.add(work);
}

#endif
