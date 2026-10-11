#ifndef PUD_BINDING_DESCENDER_HPP
#define PUD_BINDING_DESCENDER_HPP

#include <vector>
#include <immer/map_transient.hpp>
#include "value_objects/pud_binding_descent.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_specialization.hpp"
#include "value_objects/framed_expr.hpp"

template<typename IGetNodeAddedSpecializations>
struct pud_binding_descender {
    pud_binding_descender(
        IGetNodeAddedSpecializations& get_node_added_specializations);
    pud_binding_descent descent_root(pud_node_id root);
    pud_binding_descent descend(const pud_binding_descent& descent, pud_node_id child);

private:
    IGetNodeAddedSpecializations& get_node_added_specializations_;
};

template<typename IGetNodeAddedSpecializations>
pud_binding_descender<IGetNodeAddedSpecializations>::pud_binding_descender(
    IGetNodeAddedSpecializations& get_node_added_specializations)
    : get_node_added_specializations_(get_node_added_specializations) {
}

template<typename IGetNodeAddedSpecializations>
pud_binding_descent pud_binding_descender<IGetNodeAddedSpecializations>::descent_root(pud_node_id root) {
    // no bindings initially, just axiom head. (no specializations)
    return pud_binding_descent{
        .bindings = {},
    };
}

template<typename IGetNodeAddedSpecializations>
pud_binding_descent pud_binding_descender<IGetNodeAddedSpecializations>::descend(const pud_binding_descent& descent, pud_node_id child) {
    auto new_bindings_transient = descent.bindings.transient();
    const auto& specializations = get_node_added_specializations_(child);
    for (const pud_specialization& specialization : specializations) {
        new_bindings_transient.insert({
            specialization.var_idx, 
            framed_expr{specialization.value, 0}});
    }
    return pud_binding_descent{
        .bindings = new_bindings_transient.persistent(),
    };
}

#endif
