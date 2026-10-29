#ifndef PUD_LODESTAR_SLOT_ID_HPP
#define PUD_LODESTAR_SLOT_ID_HPP

#include <cstdint>
#include <compare>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"

inline constexpr uint8_t k_lodestar_side_a = 0;
inline constexpr uint8_t k_lodestar_side_b = 1;
inline constexpr uint8_t k_lodestar_side_self = 2;
inline constexpr uint8_t k_lodestar_side_caller = 3;

struct pud_lodestar_slot_id {
    pud_global_node_id query_node;
    pud_body_goal_id query_goal;
    pud_mhcs_head_id candidate;
    uint8_t side;
    auto operator<=>(const pud_lodestar_slot_id&) const = default;
};

#endif
