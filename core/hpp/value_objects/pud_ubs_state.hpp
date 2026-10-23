#ifndef PUD_UBS_STATE_HPP
#define PUD_UBS_STATE_HPP

#include <cstdint>
#include <immer/map.hpp>
#include "value_objects/ubs_class.hpp"

// A persistent union-by-size environment: the parent of every absorbed root, and the class of every
// root that is not an unbound singleton. immer maps define equality but no ordering, so the state
// is equality-comparable only.
struct pud_ubs_state {
    immer::map<uint32_t, uint32_t> parents;
    immer::map<uint32_t, ubs_class> classes;
    bool operator==(const pud_ubs_state&) const = default;
};

#endif
