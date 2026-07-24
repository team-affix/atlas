#ifndef QUELL_RESOLVER_HPP
#define QUELL_RESOLVER_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IResolver, typename IGetRule, typename IGetGoalWorkValue,
         typename IGetGoalDepth, typename IGetGoalWork,
         typename ISubtractRemainingWork, typename IAddRemainingWork>
struct quell_resolver {
    quell_resolver(IResolver&, IGetRule&, IGetGoalWorkValue&, IGetGoalDepth&,
                   IGetGoalWork&, ISubtractRemainingWork&, IAddRemainingWork&);
    bool resolve(const resolution_lineage*);
private:
    IResolver& resolver_;
    IGetRule& get_rule_;
    IGetGoalWorkValue& get_goal_work_value_;
    IGetGoalDepth& get_goal_depth_;
    IGetGoalWork& get_goal_work_;
    ISubtractRemainingWork& subtract_remaining_work_;
    IAddRemainingWork& add_remaining_work_;
};

template<typename IR, typename IGR, typename IGGWV, typename IGGD, typename IGGW,
         typename ISRW, typename IARW>
quell_resolver<IR, IGR, IGGWV, IGGD, IGGW, ISRW, IARW>::quell_resolver(
    IR& r, IGR& db, IGGWV& ggwv, IGGD& ggd, IGGW& ggw, ISRW& srw, IARW& arw)
    : resolver_(r)
    , get_rule_(db)
    , get_goal_work_value_(ggwv)
    , get_goal_depth_(ggd)
    , get_goal_work_(ggw)
    , subtract_remaining_work_(srw)
    , add_remaining_work_(arw) {}

template<typename IR, typename IGR, typename IGGWV, typename IGGD, typename IGGW,
         typename ISRW, typename IARW>
bool quell_resolver<IR, IGR, IGGWV, IGGD, IGGW, ISRW, IARW>::resolve(
    const resolution_lineage* rl) {
    const goal_lineage* parent = rl->parent;
    const double parent_work = get_goal_work_value_.get(parent);
    const size_t parent_depth = get_goal_depth_.get(parent);
    const rule* rule = get_rule_.get_rule(rl->idx);
    const size_t child_depth = parent_depth + 1;
    const double child_work = get_goal_work_.get(child_depth);
    const double children_work =
        child_work * static_cast<double>(rule->body.size());
    if (!resolver_.resolve(rl))
        return false;
    subtract_remaining_work_.subtract(parent_work);
    add_remaining_work_.add(children_work);
    return true;
}

#endif
