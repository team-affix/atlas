#ifndef PUD_CANDIDATE_SEARCH_CONTEXT_HPP
#define PUD_CANDIDATE_SEARCH_CONTEXT_HPP

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_pair.hpp"

struct pud_candidate_search_context {
    const pud_rule_id* query_leaf;
    size_t body_goal_idx;
    uint32_t frame_offset;
    const pud_rule_id* cursor;
    std::optional<pud_witness_pair> witnesses;
    auto operator<=>(const pud_candidate_search_context&) const = default;
};

#endif
