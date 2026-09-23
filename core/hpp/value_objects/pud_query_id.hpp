#ifndef PUD_QUERY_ID_HPP
#define PUD_QUERY_ID_HPP

#include <compare>
#include <cstddef>
#include "value_objects/pud_rule_id.hpp"

struct pud_query_id {
    const pud_rule_id* rule;
    size_t body_goal_idx;
    auto operator<=>(const pud_query_id&) const = default;
};

#endif
