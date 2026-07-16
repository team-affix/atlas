#ifndef QUERIER_HPP
#define QUERIER_HPP

#include "infrastructure/rule_id_set.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

template<typename IGetGoalExpr, typename ILookupAllRules, typename ILookupRuleByOutermostFunctor>
struct querier {
    querier(IGetGoalExpr&, ILookupAllRules&, ILookupRuleByOutermostFunctor&);
    const rule_id_set& get_candidate_rules(const goal_lineage* gl) const;
private:
    IGetGoalExpr& get_goal_expr_;
    ILookupAllRules& lookup_all_rules_;
    ILookupRuleByOutermostFunctor& lookup_rule_by_outermost_functor_;
};

template<typename IGGE, typename ILAR, typename ILRBOF>
querier<IGGE, ILAR, ILRBOF>::querier(IGGE& gge, ILAR& lar, ILRBOF& lrbof)
    : get_goal_expr_(gge), lookup_all_rules_(lar), lookup_rule_by_outermost_functor_(lrbof) {}

template<typename IGGE, typename ILAR, typename ILRBOF>
const rule_id_set& querier<IGGE, ILAR, ILRBOF>::get_candidate_rules(const goal_lineage* gl) const {
    framed_expr fe = get_goal_expr_.get(gl);
    if (std::holds_alternative<expr::var>(fe.skeleton->content))
        return lookup_all_rules_.lookup_all_rules();
    const expr::functor* f = std::get_if<expr::functor>(&fe.skeleton->content);
    DEBUG_ASSERT(f != nullptr);
    return lookup_rule_by_outermost_functor_.lookup_rule_by_outermost_functor(f->id);
}

#endif
