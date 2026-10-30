#ifndef MODULED_EXPR_HPP
#define MODULED_EXPR_HPP

#include <compare>
#include <cstdint>
#include "value_objects/expr.hpp"
#include "value_objects/pud_module_id.hpp"

struct moduled_expr {
    const expr* skeleton;
    uint32_t frame_offset;
    pud_module_id module;
    auto operator<=>(const moduled_expr&) const = default;
};

#endif
