#ifndef PUD_WATCHED_CANDIDATE_HPP
#define PUD_WATCHED_CANDIDATE_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhws_head_id.hpp"

// What a candidate rests on: either its own leaf (self witness) or the witness searches it added.
struct pud_watched_candidate {
    bool self_witness;
    pud_global_node_id self_leaf;
    std::vector<pud_mhws_head_id> witnesses;
    auto operator<=>(const pud_watched_candidate&) const = default;
};

#endif
