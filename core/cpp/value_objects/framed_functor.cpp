#include "value_objects/framed_functor.hpp"
#include <variant>
#include "debug_assert.hpp"

framed_functor framed_functor_from_framed_expr(framed_expr value) {
    DEBUG_ASSERT(std::holds_alternative<expr::functor>(value.skeleton->content));
    const expr::functor* functor = std::get_if<expr::functor>(&value.skeleton->content);
    return framed_functor{value.skeleton, functor, value.frame_offset};
}

framed_expr framed_expr_from_framed_functor(const framed_functor& bound) {
    return framed_expr{bound.parent, bound.frame_offset};
}
