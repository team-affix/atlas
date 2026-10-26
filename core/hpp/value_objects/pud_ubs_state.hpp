#ifndef PUD_UBS_STATE_HPP
#define PUD_UBS_STATE_HPP

#include <cstdint>
#include <immer/map.hpp>
#include "value_objects/ubs_class.hpp"

struct pud_ubs_state {
    immer::map<uint32_t, uint32_t> parents;
    immer::map<uint32_t, ubs_class> classes;
    bool operator==(const pud_ubs_state&) const = default;
};

#endif
