#ifndef QUERIER_HPP
#define QUERIER_HPP

#include "infrastructure/rule_id_set.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"

template<typename IGetGoalExpr, typename IQuery>
struct querier {
    querier(IGetGoalExpr&, IQuery&);
    const rule_id_set& get_candidate_rules(const goal_lineage* gl) const;
private:
    IGetGoalExpr& get_goal_expr_;
    IQuery&       query_;
};

template<typename IGGE, typename IQ>
querier<IGGE, IQ>::querier(IGGE& gge, IQ& q)
    : get_goal_expr_(gge), query_(q) {}

template<typename IGGE, typename IQ>
const rule_id_set& querier<IGGE, IQ>::get_candidate_rules(const goal_lineage* gl) const {
    return query_.get_candidate_rules(get_goal_expr_.get(gl));
}

#endif
