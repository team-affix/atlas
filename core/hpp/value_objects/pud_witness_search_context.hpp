#ifndef PUD_WITNESS_SEARCH_CONTEXT_HPP
#define PUD_WITNESS_SEARCH_CONTEXT_HPP

#include <compare>
#include <cstddef>
#include <cstdint>
#include "value_objects/expr.hpp"
#include "value_objects/pud_rule_id.hpp"

struct pud_witness_search_context {
    const pud_rule_id* query_leaf;
    size_t body_goal_idx;
    const expr* body_goal;
    uint32_t frame_offset;
    const pud_rule_id* search_root;
    const pud_rule_id* current;
    auto operator<=>(const pud_witness_search_context&) const = default;
};

#endif
