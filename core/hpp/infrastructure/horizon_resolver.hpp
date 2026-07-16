#ifndef HORIZON_RESOLVER_HPP
#define HORIZON_RESOLVER_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IResolver, typename IGetRule, typename IGetGoalWeight, typename IAccumulateGroundedWeight>
struct horizon_resolver {
    horizon_resolver(IResolver&, IGetRule&, IGetGoalWeight&, IAccumulateGroundedWeight&);
    bool resolve(const resolution_lineage*);
private:
    IResolver& resolver_;
    IGetRule& get_rule_;
    IGetGoalWeight& goal_weights_;
    IAccumulateGroundedWeight& cumulative_grounded_weight_;
};

template<typename IR, typename IGR, typename IGGW, typename IAGW>
horizon_resolver<IR,IGR,IGGW,IAGW>::horizon_resolver(IR& r, IGR& db, IGGW& gw, IAGW& cgw)
    : resolver_(r), get_rule_(db), goal_weights_(gw), cumulative_grounded_weight_(cgw) {}

template<typename IR, typename IGR, typename IGGW, typename IAGW>
bool horizon_resolver<IR,IGR,IGGW,IAGW>::resolve(const resolution_lineage* rl) {
    const rule* rule = get_rule_.get_rule(rl->idx);
    if (rule->body.empty()) {
        const double w = goal_weights_.get(rl->parent);
        cumulative_grounded_weight_.accumulate(w);
    }
    return resolver_.resolve(rl);
}

#endif
