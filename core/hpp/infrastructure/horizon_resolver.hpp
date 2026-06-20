#ifndef HORIZON_RESOLVER_HPP
#define HORIZON_RESOLVER_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IResolver, typename IDb, typename IGoalWeights, typename ICumulativeGroundedWeight>
struct horizon_resolver {
    horizon_resolver(IResolver&, IDb&, IGoalWeights&, ICumulativeGroundedWeight&);
    bool resolve(const resolution_lineage*);
private:
    IResolver& resolver_;
    IDb& get_rule_;
    IGoalWeights& goal_weights_;
    ICumulativeGroundedWeight& cumulative_grounded_weight_;
};

template<typename IR, typename IDB, typename IGW, typename ICGW>
horizon_resolver<IR,IDB,IGW,ICGW>::horizon_resolver(IR& r, IDB& db, IGW& gw, ICGW& cgw)
    : resolver_(r), get_rule_(db), goal_weights_(gw), cumulative_grounded_weight_(cgw) {}

template<typename IR, typename IDB, typename IGW, typename ICGW>
bool horizon_resolver<IR,IDB,IGW,ICGW>::resolve(const resolution_lineage* rl) {
    const rule* rule = get_rule_.get(rl->idx);
    if (rule->body.empty()) {
        const double w = goal_weights_.get(rl->parent);
        cumulative_grounded_weight_.accumulate(w);
    }
    return resolver_.resolve(rl);
}

#endif
