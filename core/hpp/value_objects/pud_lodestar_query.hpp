#ifndef PUD_LODESTAR_QUERY_HPP
#define PUD_LODESTAR_QUERY_HPP

#include <compare>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

// One body goal of a live, unrefuted leaf in the conjecture module, with the query it holds open.
struct pud_lodestar_query {
    pud_global_node_id node;
    pud_body_goal_id goal;
    pud_mhq_head_id head;
    bool has_candidates;
    auto operator<=>(const pud_lodestar_query&) const = default;
};

#endif
