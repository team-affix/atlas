#ifndef PUD_UBS_STATE_HPP
#define PUD_UBS_STATE_HPP

#include <cstdint>
#include <immer/map.hpp>
#include "value_objects/expr.hpp"

struct pud_ubs_state {
    immer::map<uint32_t, uint32_t> parents;
    immer::map<uint32_t, uint32_t> sizes;
    immer::map<uint32_t, const expr*> values;
    auto operator<=>(const pud_ubs_state&) const = default;
};

#endif
