#ifndef PUD_UNFOLD_HPP
#define PUD_UNFOLD_HPP

#include <compare>
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"

// One unfold: a leaf and the open goal it is unfolded over, whether forced or decided.
struct pud_unfold {
    pud_node_id leaf;
    body_goal_id goal;
    auto operator<=>(const pud_unfold&) const = default;
};

#endif
