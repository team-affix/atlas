#ifndef PUD_LODESTAR_SCHEDULE_HPP
#define PUD_LODESTAR_SCHEDULE_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/pud_lodestar_choice.hpp"

struct pud_lodestar_schedule {
    pud_lodestar_schedule();
    std::optional<pud_lodestar_choice> next_non_conjecture(const std::vector<pud_lodestar_choice>& ring);
    std::optional<pud_lodestar_choice> next_conjecture(const std::vector<pud_lodestar_choice>& ring);
    void reset_ratio();
    size_t non_conjecture_since_caller() const;
    void count_non_conjecture();
private:
    std::optional<pud_lodestar_choice> next_after(
        const std::vector<pud_lodestar_choice>& ring,
        std::optional<pud_lodestar_slot_id>& cursor) const;

    std::optional<pud_lodestar_slot_id> non_conjecture_cursor_;
    std::optional<pud_lodestar_slot_id> conjecture_cursor_;
    size_t non_conjecture_since_caller_;
};

#endif
