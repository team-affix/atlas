#ifndef PUD_SPECIALIZATION_STATE_HPP
#define PUD_SPECIALIZATION_STATE_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_witness_search_stack.hpp"

// A witness of a candidate: the child replay its search started from, the search, and the leaf it found.
struct pud_specialization_witness {
    pud_replay search_root;
    pud_witness_search_stack search;
    pud_global_node_id leaf;
    bool operator==(const pud_specialization_witness&) const = default;
};

// The two witnesses a candidate keeps in different child subtrees, and the next child to look for one under.
struct pud_specialization_witnesses {
    std::optional<pud_specialization_witness> a;
    std::optional<pud_specialization_witness> b;
    size_t next_child;
    bool operator==(const pud_specialization_witnesses&) const = default;
};

// A candidate being specialized: the path from its axiom root down to the node it stands on, that node's replay,
// and its witnesses, none while it stands on a leaf, which is then its own witness.
struct pud_specialization_state {
    std::vector<pud_global_node_id> path;
    pud_replay current;
    std::optional<pud_specialization_witnesses> witnesses;
    bool operator==(const pud_specialization_state&) const = default;
};

#endif
