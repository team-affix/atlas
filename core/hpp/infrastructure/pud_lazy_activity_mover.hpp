#ifndef PUD_LAZY_ACTIVITY_MOVER_HPP
#define PUD_LAZY_ACTIVITY_MOVER_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// Moves activity from an unfolded leaf to its children when nodes keep no active fields: their states are worked out
// when read, so there is nothing to set up or tear down, and no child arrives refuted.
struct pud_lazy_activity_mover {
    pud_lazy_activity_mover();
    std::vector<pud_global_node_id> move(pud_global_node_id parent, const std::vector<pud_global_node_id>& children);
};

#endif
