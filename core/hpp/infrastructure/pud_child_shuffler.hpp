#ifndef PUD_CHILD_SHUFFLER_HPP
#define PUD_CHILD_SHUFFLER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_inference.hpp"

// Puts an unfold's children in a pseudo-random order before they are stored, so every walk over a node's
// children (witness searches, activation, leaf finding) follows an order that does not depend on rule order.
// The order is a Fisher-Yates shuffle driven by splitmix64 seeded from the parent's id: the same parent always
// gets the same order, on every platform, and stored modules keep it.
struct pud_child_shuffler {
    std::vector<pud_inference> shuffle(pud_global_node_id parent, std::vector<pud_inference> children) const;
private:
    static uint64_t next_random(uint64_t& state);
};

#endif
