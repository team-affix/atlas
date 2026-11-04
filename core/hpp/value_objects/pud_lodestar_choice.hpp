#ifndef PUD_LODESTAR_CHOICE_HPP
#define PUD_LODESTAR_CHOICE_HPP

#include <compare>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_lodestar_slot_id.hpp"

struct pud_lodestar_choice {
    pud_lodestar_slot_id slot;
    pud_global_node_id leaf;
    auto operator<=>(const pud_lodestar_choice&) const = default;
};

#endif
