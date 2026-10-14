#ifndef PUD_DESCENDER_STACK_HPP
#define PUD_DESCENDER_STACK_HPP

#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_pool_ubs_factories.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_descender.hpp"
#include "infrastructure/ubs_transient_bind_map_data.hpp"

struct pud_descender_stack {
    expr_pool exprs;
    globalizer globalize;
    expr_pool_ubs_var_factory ubs_var_factory{exprs};
    expr_pool_ubs_functor_factory ubs_functor_factory{exprs};

    template<
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
    using bind_map_t = ubs_transient_bind_map<expr_pool_ubs_var_factory, expr_pool_ubs_functor_factory>;

    using descender_t = pud_descender<
        bind_map_t,
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

    template<typename DescenderT, typename... Args>
    DescenderT make_descender(Args&... args) {
        return DescenderT{
            args...,
            ubs_var_factory,
            ubs_functor_factory,
            exprs,
            exprs,
            globalize};
    }
};

#endif
