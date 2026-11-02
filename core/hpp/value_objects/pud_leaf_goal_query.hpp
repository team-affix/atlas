#ifndef PUD_LEAF_GOAL_QUERY_HPP
#define PUD_LEAF_GOAL_QUERY_HPP

#include <compare>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

// One body goal of a live leaf, with the query the leaf holds open for it and whether that query still
// has candidates.
struct pud_leaf_goal_query {
    pud_global_node_id node;
    pud_body_goal_id goal;
    pud_mhq_head_id head;
    bool has_candidates;
    auto operator<=>(const pud_leaf_goal_query&) const = default;
};

#endif
