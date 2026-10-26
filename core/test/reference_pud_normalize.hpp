#ifndef REFERENCE_PUD_NORMALIZE_HPP
#define REFERENCE_PUD_NORMALIZE_HPP

#include <cstddef>
#include <variant>
#include <vector>
#include "pud_term_stack.hpp"
#include "value_objects/expr.hpp"

// Only valid when every bound value was bound at frame offset 0: a value's own frame is not applied.
inline const expr* reference_pud_normalize(pud_term_stack& stack, const expr* skeleton) {
    if (const expr::var* variable = std::get_if<expr::var>(&skeleton->content)) {
        const ubs_class variable_class = stack.bind_map.find(variable->index);
        if (!variable_class.value.has_value())
            return stack.pool.make_var(variable_class.rep);
        skeleton = reinterpret_cast<const expr*>(
            reinterpret_cast<const char*>(variable_class.value->functor) - offsetof(expr, content));
    }
    const expr::functor& term = std::get<expr::functor>(skeleton->content);
    std::vector<const expr*> args;
    for (const expr* arg : term.args)
        args.push_back(reference_pud_normalize(stack, arg));
    return stack.pool.make_functor(term.id, args);
}

#endif
