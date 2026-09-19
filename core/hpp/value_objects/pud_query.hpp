#ifndef PUD_QUERY_HPP
#define PUD_QUERY_HPP

#include <cstdint>
#include <compare>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"

struct pud_query {
    om_interval interval;
    const expr* body_goal;
    std::vector<pud_candidate_search_context> axiom_contexts;
    uint32_t frame_offset;
    std::strong_ordering operator<=>(const pud_query& other) const;
    bool operator==(const pud_query& other) const;
};

#endif
