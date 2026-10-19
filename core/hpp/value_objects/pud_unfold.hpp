#ifndef PUD_UNFOLD_HPP
#define PUD_UNFOLD_HPP

#include <compare>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"

struct pud_unfold {
    pud_node_id leaf;
    pud_body_goal_id goal;
    auto operator<=>(const pud_unfold&) const = default;
};

#endif
