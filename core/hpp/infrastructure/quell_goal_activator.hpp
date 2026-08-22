#ifndef QUELL_GOAL_ACTIVATOR_HPP
#define QUELL_GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IGoalActivator, typename ISetGoalWorkValue, typename IGetGoalWork,
         typename IAddRemainingWork>
struct quell_goal_activator {
    quell_goal_activator(IGoalActivator&, ISetGoalWorkValue&, IGetGoalWork&,
                         IAddRemainingWork&);
    void activate(const goal_lineage*);
private:
    IGoalActivator& goal_activator_;
    ISetGoalWorkValue& set_goal_work_value_;
    IGetGoalWork& get_goal_work_;
    IAddRemainingWork& add_remaining_work_;
};

template<typename IGA, typename ISGWV, typename IGGW, typename IARW>
quell_goal_activator<IGA, ISGWV, IGGW, IARW>::quell_goal_activator(
    IGA& ga, ISGWV& sgwv, IGGW& ggw, IARW& arw)
    : goal_activator_(ga)
    , set_goal_work_value_(sgwv)
    , get_goal_work_(ggw)
    , add_remaining_work_(arw) {}

template<typename IGA, typename ISGWV, typename IGGW, typename IARW>
void quell_goal_activator<IGA, ISGWV, IGGW, IARW>::activate(const goal_lineage* gl) {
    goal_activator_.activate(gl);
    const double work = get_goal_work_.get(gl);
    set_goal_work_value_.set(gl, work);
    add_remaining_work_.add(work);
}

#endif
