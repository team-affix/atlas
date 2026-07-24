#ifndef QUELL_RESOLVER_HPP
#define QUELL_RESOLVER_HPP

#include "value_objects/lineage.hpp"

template<typename IResolver, typename IGetGoalWorkValue, typename ISubtractRemainingWork>
struct quell_resolver {
    quell_resolver(IResolver&, IGetGoalWorkValue&, ISubtractRemainingWork&);
    bool resolve(const resolution_lineage*);
private:
    IResolver& resolver_;
    IGetGoalWorkValue& get_goal_work_value_;
    ISubtractRemainingWork& subtract_remaining_work_;
};

template<typename IR, typename IGGWV, typename ISRW>
quell_resolver<IR, IGGWV, ISRW>::quell_resolver(
    IR& r, IGGWV& ggwv, ISRW& srw)
    : resolver_(r)
    , get_goal_work_value_(ggwv)
    , subtract_remaining_work_(srw) {}

template<typename IR, typename IGGWV, typename ISRW>
bool quell_resolver<IR, IGGWV, ISRW>::resolve(const resolution_lineage* rl) {
    const double parent_work = get_goal_work_value_.get(rl->parent);
    if (!resolver_.resolve(rl))
        return false;
    // Children already credited remaining_work in quell_goal_activator.
    subtract_remaining_work_.subtract(parent_work);
    return true;
}

#endif
