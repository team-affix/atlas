#include "value_objects/framed_functor.hpp"
#include <cstddef>

framed_expr framed_expr_from_framed_functor(const framed_functor& bound) {
    const expr* parent = reinterpret_cast<const expr*>(
        reinterpret_cast<const char*>(bound.functor) - offsetof(expr, content));
    return framed_expr{parent, bound.frame_offset};
}
