#ifndef PUD_DECISION_HPP
#define PUD_DECISION_HPP

#include <compare>
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Which live leaf to unfold, and over which of its open body goals.
struct pud_decision {
    pud_node_id leaf;
    body_goal_id goal;
    auto operator<=>(const pud_decision&) const = default;
};

#endif
