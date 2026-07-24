#ifndef QUELL_INITIAL_GOAL_ACTIVATOR_HPP
#define QUELL_INITIAL_GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IInitialGoalActivator, typename IMakeInitialGoalLineage,
         typename ISetGoalDepth, typename ISetGoalWorkValue,
         typename IGetGoalWork, typename IAddRemainingWork>
struct quell_initial_goal_activator {
    quell_initial_goal_activator(IInitialGoalActivator&, IMakeInitialGoalLineage&,
                                 ISetGoalDepth&, ISetGoalWorkValue&,
                                 IGetGoalWork&, IAddRemainingWork&);
    void activate_initial_goal(subgoal_id idx);
private:
    IInitialGoalActivator& initial_goal_activator_;
    IMakeInitialGoalLineage& make_initial_goal_lineage_;
    ISetGoalDepth& set_goal_depth_;
    ISetGoalWorkValue& set_goal_work_value_;
    IGetGoalWork& get_goal_work_;
    IAddRemainingWork& add_remaining_work_;
};

template<typename IIGA, typename IMIGL, typename ISGD, typename ISGWV,
         typename IGGW, typename IARW>
quell_initial_goal_activator<IIGA, IMIGL, ISGD, ISGWV, IGGW, IARW>::
quell_initial_goal_activator(
    IIGA& iga, IMIGL& migl, ISGD& sgd, ISGWV& sgwv, IGGW& ggw, IARW& arw)
    : initial_goal_activator_(iga)
    , make_initial_goal_lineage_(migl)
    , set_goal_depth_(sgd)
    , set_goal_work_value_(sgwv)
    , get_goal_work_(ggw)
    , add_remaining_work_(arw) {}

template<typename IIGA, typename IMIGL, typename ISGD, typename ISGWV,
         typename IGGW, typename IARW>
void quell_initial_goal_activator<IIGA, IMIGL, ISGD, ISGWV, IGGW, IARW>::
activate_initial_goal(subgoal_id idx) {
    initial_goal_activator_.activate_initial_goal(idx);
    const goal_lineage* gl = make_initial_goal_lineage_.make(idx);
    const size_t depth = 0;
    const double work = get_goal_work_.get(depth);
    set_goal_depth_.set(gl, depth);
    set_goal_work_value_.set(gl, work);
    add_remaining_work_.add(work);
}

#endif
