#ifndef PUD_UNFOLDED_HPP
#define PUD_UNFOLDED_HPP

#include <compare>
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"

struct pud_unfolded {
    pud_node_id leaf;
    body_goal_id goal;
    auto operator<=>(const pud_unfolded&) const = default;
};

#endif
