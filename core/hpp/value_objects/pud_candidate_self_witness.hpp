#ifndef PUD_CANDIDATE_SELF_WITNESS_HPP
#define PUD_CANDIDATE_SELF_WITNESS_HPP

#include <compare>
#include "value_objects/pud_node.hpp"

struct pud_candidate_self_witness {
    const pud_node* node;
    auto operator<=>(const pud_candidate_self_witness&) const = default;
};

#endif
