#ifndef PUD_WITNESS_ADVANCE_HPP
#define PUD_WITNESS_ADVANCE_HPP

#include <cstddef>
#include "value_objects/pud_replay.hpp"

// A witness search moved one level down: its new search root, and the position among the old root's
// children just after that new root.
struct pud_witness_advance {
    pud_replay search_root;
    size_t next_sibling;
    bool operator==(const pud_witness_advance&) const = default;
};

#endif
