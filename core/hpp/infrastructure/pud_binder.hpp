#ifndef PUD_BINDER_HPP
#define PUD_BINDER_HPP

#include <optional>
#include <utility>
#include <variant>
#include "value_objects/expr.hpp"
#include "value_objects/framed_functor.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/ubs_class.hpp"
#include "debug_assert.hpp"

// Gives a child the bindings of its rule: its parent's, with the child's substitutions applied. A
// substitution targets a representative unbound in the parent, so applying it never displaces a value.
template<typename IMakeBindMap, typename IGetInference>
struct pud_binder {
    pud_binder(IMakeBindMap& make_bind_map, IGetInference& get_inference);
    pud_ubs_state bind(const pud_ubs_state& parent_bindings, pud_global_node_id child) const;
private:
    IMakeBindMap& make_bind_map_;
    IGetInference& get_inference_;
};

template<typename IMBM, typename IGI>
pud_binder<IMBM, IGI>::pud_binder(IMBM& make_bind_map, IGI& get_inference)
    : make_bind_map_(make_bind_map)
    , get_inference_(get_inference) {}

template<typename IMBM, typename IGI>
pud_ubs_state pud_binder<IMBM, IGI>::bind(const pud_ubs_state& parent_bindings, pud_global_node_id child) const {
    auto&& bind_map = make_bind_map_.make_bind_map(parent_bindings);
    for (const pud_substitution& substitution : get_inference_.get_inference(child).substitutions) {
        const ubs_class target = bind_map.find(substitution.var_idx);
        if (const expr::var* var = std::get_if<expr::var>(&substitution.value->content)) {
            [[maybe_unused]] const std::optional<framed_functor> displaced = bind_map.equate(target, bind_map.find(var->index));
            DEBUG_ASSERT(!displaced.has_value());
            continue;
        }
        bind_map.instantiate(target, framed_functor{std::get_if<expr::functor>(&(substitution.value)->content), 0});
    }
    return std::move(bind_map).persist();
}

#endif
