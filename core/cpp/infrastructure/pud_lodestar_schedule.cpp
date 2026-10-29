#include "infrastructure/pud_lodestar_schedule.hpp"

pud_lodestar_schedule::pud_lodestar_schedule()
    : non_conjecture_cursor_(std::nullopt)
    , conjecture_cursor_(std::nullopt)
    , non_conjecture_since_caller_(0) {}

std::optional<pud_lodestar_choice> pud_lodestar_schedule::next_after(
    const std::vector<pud_lodestar_choice>& ring,
    std::optional<pud_lodestar_slot_id>& cursor) const {
    const bool wrapped = !cursor.has_value();
    std::optional<pud_lodestar_choice> first_eligible;
    for (const pud_lodestar_choice& choice : ring) {
        if (!choice.eligible)
            continue;
        if (!first_eligible.has_value())
            first_eligible = choice;
        if (wrapped)
            return choice;
        if (cursor.value() < choice.slot)
            return choice;
    }
    if (!wrapped && first_eligible.has_value() && first_eligible->slot != cursor.value())
        return first_eligible;
    if (wrapped)
        return first_eligible;
    return first_eligible;
}

std::optional<pud_lodestar_choice> pud_lodestar_schedule::next_non_conjecture(
    const std::vector<pud_lodestar_choice>& ring) {
    const std::optional<pud_lodestar_choice> chosen = next_after(ring, non_conjecture_cursor_);
    if (!chosen.has_value())
        return std::nullopt;
    non_conjecture_cursor_ = chosen->slot;
    return chosen;
}

std::optional<pud_lodestar_choice> pud_lodestar_schedule::next_conjecture(
    const std::vector<pud_lodestar_choice>& ring) {
    const std::optional<pud_lodestar_choice> chosen = next_after(ring, conjecture_cursor_);
    if (!chosen.has_value())
        return std::nullopt;
    conjecture_cursor_ = chosen->slot;
    return chosen;
}

void pud_lodestar_schedule::reset_ratio() {
    non_conjecture_since_caller_ = 0;
}

size_t pud_lodestar_schedule::non_conjecture_since_caller() const {
    return non_conjecture_since_caller_;
}

void pud_lodestar_schedule::count_non_conjecture() {
    ++non_conjecture_since_caller_;
}
