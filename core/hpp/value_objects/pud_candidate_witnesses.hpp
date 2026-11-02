#ifndef PUD_CANDIDATE_WITNESSES_HPP
#define PUD_CANDIDATE_WITNESSES_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"

// What a candidate rests on: either its own leaf (self witness) or the witness searches it added.
struct pud_candidate_witnesses {
    bool self_witness;
    pud_global_node_id self_leaf;
    std::vector<pud_mhws_head_id> witnesses;
    auto operator<=>(const pud_candidate_witnesses&) const = default;
};

#endif
