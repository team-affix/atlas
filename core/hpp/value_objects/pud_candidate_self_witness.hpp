#ifndef PUD_CANDIDATE_SELF_WITNESS_HPP
#define PUD_CANDIDATE_SELF_WITNESS_HPP

#include <compare>
#include "value_objects/pud_node_id.hpp"

struct pud_candidate_self_witness {
    pud_node_id node;
    auto operator<=>(const pud_candidate_self_witness&) const = default;
};

#endif
