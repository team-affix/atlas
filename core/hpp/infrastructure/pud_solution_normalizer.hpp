#ifndef PUD_SOLUTION_NORMALIZER_HPP
#define PUD_SOLUTION_NORMALIZER_HPP

#include <cstdint>
#include <variant>
#include <vector>
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/ubs_class.hpp"

template<typename IGetGlobalFunctorIdFromLocalFunctorId, typename IMakeVar, typename IMakeFunctor>
struct pud_solution_normalizer {
    pud_solution_normalizer(
        IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
        IMakeVar& make_var,
        IMakeFunctor& make_functor);
    const expr* normalize(const expr* skeleton, pud_module_id module, const pud_ubs_state& bindings) const;
private:
    const expr* walk(const ubs_bind_map& bind_map, const expr* skeleton, pud_module_id module) const;

    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
};

template<typename IGetGlobalFunctorIdFromLocalFunctorId, typename IMakeVar, typename IMakeFunctor>
pud_solution_normalizer<IGetGlobalFunctorIdFromLocalFunctorId, IMakeVar, IMakeFunctor>::pud_solution_normalizer(
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
    IMakeVar& make_var,
    IMakeFunctor& make_functor)
    : get_global_functor_id_(get_global_functor_id)
    , make_var_(make_var)
    , make_functor_(make_functor) {}

template<typename IGetGlobalFunctorIdFromLocalFunctorId, typename IMakeVar, typename IMakeFunctor>
const expr* pud_solution_normalizer<IGetGlobalFunctorIdFromLocalFunctorId, IMakeVar, IMakeFunctor>::normalize(
    const expr* skeleton, pud_module_id module, const pud_ubs_state& bindings) const {
    const ubs_bind_map bind_map{bindings};
    return walk(bind_map, skeleton, module);
}

template<typename IGetGlobalFunctorIdFromLocalFunctorId, typename IMakeVar, typename IMakeFunctor>
const expr* pud_solution_normalizer<IGetGlobalFunctorIdFromLocalFunctorId, IMakeVar, IMakeFunctor>::walk(
    const ubs_bind_map& bind_map, const expr* skeleton, pud_module_id module) const {
    if (const expr::var* variable = std::get_if<expr::var>(&skeleton->content)) {
        const ubs_class variable_class = bind_map.find(variable->index);
        if (!variable_class.value.has_value())
            return make_var_.make_var(variable_class.rep);
        const expr bound_term{expr::functor{*variable_class.value->functor}};
        return walk(bind_map, &bound_term, variable_class.value->module);
    }
    const expr::functor& term = std::get<expr::functor>(skeleton->content);
    const uint32_t global_functor_id = get_global_functor_id_.get_global_functor_id_from_local_functor_id(
        module, term.id);
    std::vector<const expr*> args;
    for (const expr* arg : term.args)
        args.push_back(walk(bind_map, arg, module));
    return make_functor_.make_functor(global_functor_id, args);
}

#endif
