#ifndef PUD_BINDING_DESCENT_HPP
#define PUD_BINDING_DESCENT_HPP

#include <variant>
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/pud_ubs_state.hpp"

template<typename IGetNodeAddedSubstitutions>
struct pud_binding_descent {
    pud_binding_descent(IGetNodeAddedSubstitutions& get_node_added_substitutions);
    pud_binding_descent descend(pud_node_id child) const;
    const pud_ubs_state& ubs_state() const;
private:
    IGetNodeAddedSubstitutions* get_node_added_substitutions_;
    pud_ubs_state ubs_state_;
};

template<typename IGNAS>
pud_binding_descent<IGNAS>::pud_binding_descent(IGNAS& get_node_added_substitutions)
    : get_node_added_substitutions_(&get_node_added_substitutions)
    , ubs_state_() {}

template<typename IGNAS>
pud_binding_descent<IGNAS> pud_binding_descent<IGNAS>::descend(pud_node_id child) const {
    pud_transient_ubs_state transient_ubs_state{ubs_state_};
    auto bind_map = transient_ubs_state.make_bind_map();
    for (const pud_substitution& substitution : get_node_added_substitutions_->get(child)) {
        if (const expr::var* var = std::get_if<expr::var>(&substitution.value->content)) {
            bind_map.equate(substitution.var_idx, var->index);
            continue;
        }
        bind_map.instantiate(substitution.var_idx, framed_expr{substitution.value, 0});
    }
    pud_binding_descent result = *this;
    result.ubs_state_ = transient_ubs_state.to_persistent_state();
    return result;
}

template<typename IGNAS>
const pud_ubs_state& pud_binding_descent<IGNAS>::ubs_state() const {
    return ubs_state_;
}

#endif
