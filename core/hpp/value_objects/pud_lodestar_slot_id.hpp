#ifndef PUD_LODESTAR_SLOT_ID_HPP
#define PUD_LODESTAR_SLOT_ID_HPP

#include <cstdint>
#include <compare>
#include "value_objects/pud_lodestar_side.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"

struct pud_lodestar_slot_id {
    pud_global_node_id query_node;
    pud_body_goal_id query_goal;
    pud_mhcs_head_id candidate;
    pud_lodestar_side side;
    auto operator<=>(const pud_lodestar_slot_id&) const = default;
};

#endif
