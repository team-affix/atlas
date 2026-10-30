#ifndef MODULED_FUNCTOR_HPP
#define MODULED_FUNCTOR_HPP

#include <compare>
#include <cstdint>
#include "value_objects/expr.hpp"
#include "value_objects/pud_module_id.hpp"

struct moduled_functor {
    const expr::functor* functor;
    uint32_t frame_offset;
    pud_module_id module;
    auto operator<=>(const moduled_functor&) const = default;
};

#endif
