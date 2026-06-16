#ifndef FRAMED_EXPR_HPP
#define FRAMED_EXPR_HPP

#include <cstdint>
#include "expr.hpp"

struct framed_expr {
    const expr* skeleton;
    uint32_t frame_offset;
    bool operator==(const framed_expr&) const = default;
};

#endif
