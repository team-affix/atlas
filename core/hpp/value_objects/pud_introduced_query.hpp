#ifndef PUD_INTRODUCED_QUERY_HPP
#define PUD_INTRODUCED_QUERY_HPP

#include <compare>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

// A query that was just introduced: its head, the node that opened or forked it, and the goal it answers there.
struct pud_introduced_query {
    pud_mhq_head_id head;
    pud_global_node_id node;
    pud_body_goal_id goal;
    auto operator<=>(const pud_introduced_query&) const = default;
};

#endif
