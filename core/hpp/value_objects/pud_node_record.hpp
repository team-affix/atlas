#ifndef PUD_NODE_RECORD_HPP
#define PUD_NODE_RECORD_HPP

#include <compare>
#include <cstdint>

// The fixed-size part of a stored node: where its inference's encoding sits in the payload (empty for an axiom), its
// parent (k_no_parent for an axiom), the contiguous range of its children, and whether it is refuted.
struct pud_node_record {
    static constexpr uint64_t k_no_parent = UINT64_MAX;
    uint64_t payload_offset;
    uint64_t payload_size;
    uint64_t parent;
    uint64_t first_child;
    uint64_t child_count;
    uint64_t refuted;
    auto operator<=>(const pud_node_record&) const = default;
};

#endif
