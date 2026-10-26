#ifndef FRAMED_FUNCTOR_HPP
#define FRAMED_FUNCTOR_HPP

#include <compare>
#include <cstdint>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

struct framed_functor {
    const expr* parent;
    const expr::functor* functor;
    uint32_t frame_offset;
    auto operator<=>(const framed_functor&) const = default;
};

framed_expr framed_expr_from_framed_functor(const framed_functor& bound);

#endif
