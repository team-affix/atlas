#include "value_objects/framed_functor.hpp"

framed_expr framed_expr_from_framed_functor(const framed_functor& bound) {
    return framed_expr{bound.parent, bound.frame_offset};
}
