#ifndef PUD_WITNESS_SEARCH_CONTEXT_HPP
#define PUD_WITNESS_SEARCH_CONTEXT_HPP

#include <compare>
#include "value_objects/pud_rule_id.hpp"

struct pud_witness_search_context {
    const pud_rule_id* edge_root;
    const pud_rule_id* current;
    auto operator<=>(const pud_witness_search_context&) const = default;
};

#endif
