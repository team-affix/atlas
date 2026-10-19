#ifndef PUD_EXPR_STRUCTURE_HPP
#define PUD_EXPR_STRUCTURE_HPP

#include <variant>
#include "value_objects/expr.hpp"

inline bool same_structure(const expr* left, const expr* right) {
    const expr::var* left_var = std::get_if<expr::var>(&left->content);
    const expr::var* right_var = std::get_if<expr::var>(&right->content);
    if (left_var || right_var)
        return left_var && right_var && left_var->index == right_var->index;
    const expr::functor& left_functor = std::get<expr::functor>(left->content);
    const expr::functor& right_functor = std::get<expr::functor>(right->content);
    if (left_functor.id != right_functor.id || left_functor.args.size() != right_functor.args.size())
        return false;
    for (size_t index = 0; index < left_functor.args.size(); ++index) {
        if (!same_structure(left_functor.args.at(index), right_functor.args.at(index)))
            return false;
    }
    return true;
}

#endif
