#ifndef REFERENCE_PUD_CANONICAL_HPP
#define REFERENCE_PUD_CANONICAL_HPP

#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

// A goal's structure under some bindings, as a token sequence: functors as their id then arity,
// unbound classes numbered by first occurrence. Two goals have the same structure exactly when
// their sequences are equal. Unbound-class tokens start at k_reference_rep_token_base, so functor
// ids must stay below it.
constexpr uint32_t k_reference_rep_token_base = 1000000;

inline void reference_pud_visit_canonical(
    const ubs_bind_map& bind_map,
    const expr* visited,
    std::unordered_map<uint32_t, uint32_t>& numbering,
    std::vector<uint32_t>& tokens) {
    if (const expr::var* var = std::get_if<expr::var>(&visited->content)) {
        const ubs_class var_class = bind_map.find(var->index);
        if (!var_class.value.has_value()) {
            const uint32_t number = numbering.emplace(var_class.rep, numbering.size()).first->second;
            tokens.push_back(k_reference_rep_token_base + number);
            return;
        }
        reference_pud_visit_canonical(bind_map, var_class.value->parent, numbering, tokens);
        return;
    }
    const expr::functor& functor = std::get<expr::functor>(visited->content);
    tokens.push_back(functor.id);
    tokens.push_back(functor.args.size());
    for (const expr* arg : functor.args)
        reference_pud_visit_canonical(bind_map, arg, numbering, tokens);
}

inline std::vector<uint32_t> reference_pud_canonical(const pud_ubs_state& bindings, const expr* goal) {
    const ubs_bind_map bind_map{bindings};
    std::unordered_map<uint32_t, uint32_t> numbering;
    std::vector<uint32_t> tokens;
    reference_pud_visit_canonical(bind_map, goal, numbering, tokens);
    return tokens;
}

#endif
