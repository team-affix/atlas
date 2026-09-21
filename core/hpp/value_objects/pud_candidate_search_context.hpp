#ifndef PUD_CANDIDATE_SEARCH_CONTEXT_HPP
#define PUD_CANDIDATE_SEARCH_CONTEXT_HPP

#include <compare>
#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"

struct pud_candidate_search_context {
    om_interval interval;
    const expr* body_goal;
    uint32_t frame_offset;
    const pud_rule_id* cursor;
    std::vector<pud_witness_search_context> live_edges;
    std::vector<const expr*> added_body_goals;
    auto operator<=>(const pud_candidate_search_context&) const = default;
};

#endif
