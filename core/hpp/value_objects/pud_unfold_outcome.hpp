#ifndef PUD_UNFOLD_OUTCOME_HPP
#define PUD_UNFOLD_OUTCOME_HPP

#include <cstdint>

enum struct pud_unfold_outcome : uint8_t {
    none,
    on_focus,
    off_focus,
};

#endif
