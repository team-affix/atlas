#ifndef PUD_BINDING_DESCENDER_HPP
#define PUD_BINDING_DESCENDER_HPP

#include <variant>
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/pud_binding_descent.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_specialization.hpp"
#include "value_objects/expr.hpp"

template<typename BindMap, typename IGetNodeAddedSpecializations, typename IMakeUbsVar, typename IMakeUbsFunctor>
struct pud_binding_descender {
    pud_binding_descender(
        IGetNodeAddedSpecializations& get_node_added_specializations,
        IMakeUbsVar& make_ubs_var,
        IMakeUbsFunctor& make_ubs_functor);
    pud_binding_descent root_descent(pud_node_id root);
    pud_binding_descent descend(
        const pud_binding_descent& descent,
        pud_node_id parent,
        pud_node_id child);

private:
    void apply_specialization(BindMap& bind_map, const pud_specialization& specialization);

    IGetNodeAddedSpecializations& get_node_added_specializations_;
    IMakeUbsVar& make_ubs_var_;
    IMakeUbsFunctor& make_ubs_functor_;
};

#define PUD_BINDING_DESCENDER_TEMPLATE                                                     \
    template<typename BindMap, typename IGetNodeAddedSpecializations, typename IMakeUbsVar, \
             typename IMakeUbsFunctor>
#define PUD_BINDING_DESCENDER                                                              \
    pud_binding_descender<BindMap, IGetNodeAddedSpecializations, IMakeUbsVar, IMakeUbsFunctor>

PUD_BINDING_DESCENDER_TEMPLATE
PUD_BINDING_DESCENDER::pud_binding_descender(
    IGetNodeAddedSpecializations& get_node_added_specializations,
    IMakeUbsVar& make_ubs_var,
    IMakeUbsFunctor& make_ubs_functor)
    : get_node_added_specializations_(get_node_added_specializations)
    , make_ubs_var_(make_ubs_var)
    , make_ubs_functor_(make_ubs_functor) {}

PUD_BINDING_DESCENDER_TEMPLATE
pud_binding_descent PUD_BINDING_DESCENDER::root_descent(pud_node_id root) {
    return pud_binding_descent{.ubs_state = {}};
}

PUD_BINDING_DESCENDER_TEMPLATE
pud_binding_descent PUD_BINDING_DESCENDER::descend(
    const pud_binding_descent& descent,
    pud_node_id parent,
    pud_node_id child) {
    pud_transient_ubs_state transient_ubs_state{descent.ubs_state};
    BindMap bind_map = transient_ubs_state.make_bind_map(make_ubs_var_, make_ubs_functor_);
    for (const pud_specialization& specialization : get_node_added_specializations_.get(child))
        apply_specialization(bind_map, specialization);
    return pud_binding_descent{.ubs_state = transient_ubs_state.to_persistent_state()};
}

PUD_BINDING_DESCENDER_TEMPLATE
void PUD_BINDING_DESCENDER::apply_specialization(
    BindMap& bind_map,
    const pud_specialization& specialization) {
    if (const expr::var* var = std::get_if<expr::var>(&specialization.value->content)) {
        bind_map.equate(specialization.var_idx, var->index);
        return;
    }
    const expr::functor& functor = std::get<expr::functor>(specialization.value->content);
    bind_map.instantiate(specialization.var_idx, functor.id, functor.args);
}

#undef PUD_BINDING_DESCENDER
#undef PUD_BINDING_DESCENDER_TEMPLATE

#endif
