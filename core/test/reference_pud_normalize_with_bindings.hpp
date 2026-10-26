#ifndef REFERENCE_PUD_NORMALIZE_WITH_BINDINGS_HPP
#define REFERENCE_PUD_NORMALIZE_WITH_BINDINGS_HPP

#include <cstddef>
#include <variant>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/ubs_class.hpp"

inline const expr* reference_pud_normalize_with_bindings(const ubs_bind_map& bind_map, expr_pool& pool, const expr* skeleton) {
    if (const expr::var* variable = std::get_if<expr::var>(&skeleton->content)) {
        const ubs_class variable_class = bind_map.find(variable->index);
        if (!variable_class.value.has_value())
            return pool.make_var(variable_class.rep);
        const expr* parent = reinterpret_cast<const expr*>(
            reinterpret_cast<const char*>(variable_class.value->functor) - offsetof(expr, content));
        return reference_pud_normalize_with_bindings(bind_map, pool, parent);
    }
    const expr::functor& term = std::get<expr::functor>(skeleton->content);
    std::vector<const expr*> args;
    for (const expr* arg : term.args)
        args.push_back(reference_pud_normalize_with_bindings(bind_map, pool, arg));
    return pool.make_functor(term.id, args);
}

#endif
