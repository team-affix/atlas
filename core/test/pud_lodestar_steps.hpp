#ifndef PUD_LODESTAR_STEPS_HPP
#define PUD_LODESTAR_STEPS_HPP

#include "infrastructure/pud_lodestar_runtime.hpp"

// Steps a Lodestar session past its unfolds to the next thing it reports (a solution or a refutation).
// Returns false when the session ends first.
inline bool advance_to_report(pud_lodestar_runtime& runtime) {
    while (runtime.next()) {
        if (runtime.last_step() != pud_search_step::unfolded)
            return true;
    }
    return false;
}

#endif
