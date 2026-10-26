#ifndef FRAMED_FUNCTOR_HPP
#define FRAMED_FUNCTOR_HPP

#include <compare>
#include <cstdint>
#include <variant>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

struct framed_functor {
    const expr::functor* functor;
    uint32_t frame_offset;
    auto operator<=>(const framed_functor&) const = default;
};

inline framed_functor framed_functor_from_framed_expr(const framed_expr& value) {
    const expr::functor* functor = std::get_if<expr::functor>(&value.skeleton->content);
    return framed_functor{functor, value.frame_offset};
}

#endif
