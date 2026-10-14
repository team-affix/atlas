#ifndef PUD_DESCENDER_TEST_TYPES_HPP
#define PUD_DESCENDER_TEST_TYPES_HPP

#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_pool_ubs_factories.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_descender.hpp"
#include "infrastructure/pud_descender_bind_factories.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"

using pud_descender_test_bind_map_t =
    ubs_transient_bind_map<expr_pool_ubs_var_factory, expr_pool_ubs_functor_factory>;

template<
    typename IBindFactories,
    typename INextNodeId,
    typename ICheckNodeRefuted,
    typename IGetCallSite,
    typename IGetAddedSpecializations,
    typename IGetAddedBodyGoals,
    typename IGetAddedVarCount,
    typename IGetAxiomHead,
    typename IStoreAddedSpecializations,
    typename IStoreAddedBodyGoals,
    typename IStoreAddedVarCount>
using pud_descender_test_t = pud_descender<
    pud_descender_test_bind_map_t,
    typename IBindFactories::changed_rep_logger_t,
    typename IBindFactories::normalizer_t,
    typename IBindFactories::unifier_t,
    typename IBindFactories::whnf_t,
    typename IBindFactories::export_normalizer_t,
    IBindFactories,
    IBindFactories,
    IBindFactories,
    IBindFactories,
    IBindFactories,
    expr_pool_ubs_var_factory,
    expr_pool_ubs_functor_factory,
    INextNodeId,
    expr_pool,
    expr_pool,
    globalizer,
    ICheckNodeRefuted,
    IGetCallSite,
    IGetAddedSpecializations,
    IGetAddedBodyGoals,
    IGetAddedVarCount,
    IGetAxiomHead,
    IStoreAddedSpecializations,
    IStoreAddedBodyGoals,
    IStoreAddedVarCount>;

#endif
