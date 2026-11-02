#include "infrastructure/pud_lodestar_round_robin.hpp"

pud_lodestar_round_robin::pud_lodestar_round_robin()
    : cursor_(std::nullopt) {}

std::optional<pud_lodestar_choice> pud_lodestar_round_robin::next(const std::vector<pud_lodestar_choice>& ring) {
    if (ring.empty())
        return std::nullopt;
    for (const pud_lodestar_choice& choice : ring) {
        const bool past_cursor = !cursor_.has_value() || cursor_.value() < choice.slot;
        if (!past_cursor)
            continue;
        cursor_ = choice.slot;
        return choice;
    }
    cursor_ = ring.front().slot;
    return ring.front();
}
