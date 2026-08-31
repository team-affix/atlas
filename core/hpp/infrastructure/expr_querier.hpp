#ifndef EXPR_QUERIER_HPP
#define EXPR_QUERIER_HPP

#include "infrastructure/rule_id_set.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

template<typename ILookupAllRules, typename ILookupRuleByOutermostFunctor>
struct expr_querier {
    expr_querier(ILookupAllRules&, ILookupRuleByOutermostFunctor&);
    const rule_id_set& get_candidate_rules(framed_expr fe);
private:
    ILookupAllRules&                    lookup_all_rules_;
    ILookupRuleByOutermostFunctor&      lookup_rule_by_outermost_functor_;
};

template<typename ILAR, typename ILRBOF>
expr_querier<ILAR, ILRBOF>::expr_querier(ILAR& lar, ILRBOF& lrbof)
    : lookup_all_rules_(lar), lookup_rule_by_outermost_functor_(lrbof) {}

template<typename ILAR, typename ILRBOF>
const rule_id_set& expr_querier<ILAR, ILRBOF>::get_candidate_rules(framed_expr fe) {
    if (std::holds_alternative<expr::var>(fe.skeleton->content))
        return lookup_all_rules_.lookup_all_rules();
    const expr::functor* f = std::get_if<expr::functor>(&fe.skeleton->content);
    DEBUG_ASSERT(f != nullptr);
    return lookup_rule_by_outermost_functor_.lookup_rule_by_outermost_functor(f->id);
}

#endif
