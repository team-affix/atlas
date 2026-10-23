#ifndef FRAMED_EXPR_HASH_HPP
#define FRAMED_EXPR_HASH_HPP

#include <cstddef>
#include "value_objects/framed_expr.hpp"

struct framed_expr_hash {
    size_t operator()(const framed_expr& framed) const noexcept;
};

#endif
