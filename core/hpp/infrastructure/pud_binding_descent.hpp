#ifndef PUD_BINDING_DESCENT_HPP
#define PUD_BINDING_DESCENT_HPP

#include <variant>
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_specialization.hpp"
#include "value_objects/pud_ubs_state.hpp"

template<typename BindMap, typename IGetNodeAddedSpecializations, typename IMakeVar, typename IMakeFunctor>
struct pud_binding_descent {
    pud_binding_descent(
        IGetNodeAddedSpecializations& get_node_added_specializations,
        IMakeVar& make_var,
        IMakeFunctor& make_functor);
    pud_binding_descent descend(pud_node_id child) const;
    const pud_ubs_state& ubs_state() const;
private:
    IGetNodeAddedSpecializations* get_node_added_specializations_;
    IMakeVar* make_var_;
    IMakeFunctor* make_functor_;
    pud_ubs_state ubs_state_;
};

#define PUD_BINDING_DESCENT_TEMPLATE                                                       \
    template<typename BindMap, typename IGetNodeAddedSpecializations, typename IMakeVar,    \
             typename IMakeFunctor>
#define PUD_BINDING_DESCENT                                                                \
    pud_binding_descent<BindMap, IGetNodeAddedSpecializations, IMakeVar, IMakeFunctor>

PUD_BINDING_DESCENT_TEMPLATE
PUD_BINDING_DESCENT::pud_binding_descent(
    IGetNodeAddedSpecializations& get_node_added_specializations,
    IMakeVar& make_var,
    IMakeFunctor& make_functor)
    : get_node_added_specializations_(&get_node_added_specializations)
    , make_var_(&make_var)
    , make_functor_(&make_functor)
    , ubs_state_() {}

PUD_BINDING_DESCENT_TEMPLATE
PUD_BINDING_DESCENT PUD_BINDING_DESCENT::descend(pud_node_id child) const {
    pud_transient_ubs_state transient_ubs_state{ubs_state_};
    BindMap bind_map = transient_ubs_state.make_bind_map(*make_var_, *make_functor_);
    for (const pud_specialization& specialization : get_node_added_specializations_->get(child)) {
        if (const expr::var* var = std::get_if<expr::var>(&specialization.value->content)) {
            bind_map.equate(specialization.var_idx, var->index);
            continue;
        }
        const expr::functor& functor = std::get<expr::functor>(specialization.value->content);
        bind_map.instantiate(specialization.var_idx, functor.id, functor.args);
    }
    PUD_BINDING_DESCENT result = *this;
    result.ubs_state_ = transient_ubs_state.to_persistent_state();
    return result;
}

PUD_BINDING_DESCENT_TEMPLATE
const pud_ubs_state& PUD_BINDING_DESCENT::ubs_state() const {
    return ubs_state_;
}

#undef PUD_BINDING_DESCENT
#undef PUD_BINDING_DESCENT_TEMPLATE

#endif
