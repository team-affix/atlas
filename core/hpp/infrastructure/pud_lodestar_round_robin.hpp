#ifndef PUD_LODESTAR_ROUND_ROBIN_HPP
#define PUD_LODESTAR_ROUND_ROBIN_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_lodestar_choice.hpp"
#include "value_objects/pud_lodestar_slot_id.hpp"

// Deals choices out of a slot-sorted ring one at a time: the first slot after the last one dealt,
// wrapping to the front.
struct pud_lodestar_round_robin {
    pud_lodestar_round_robin();
    std::optional<pud_lodestar_choice> next(const std::vector<pud_lodestar_choice>& ring);
private:
    std::optional<pud_lodestar_slot_id> cursor_;
};

#endif
