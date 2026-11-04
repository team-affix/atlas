#ifndef PUD_QUERY_INTRODUCTION_HPP
#define PUD_QUERY_INTRODUCTION_HPP

#include <compare>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"

// Where a query was introduced: the node that opened or forked it, and the goal it answers there.
struct pud_query_introduction {
    pud_global_node_id node;
    pud_body_goal_id goal;
    auto operator<=>(const pud_query_introduction&) const = default;
};

#endif
