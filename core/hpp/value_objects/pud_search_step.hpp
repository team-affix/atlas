#ifndef PUD_SEARCH_STEP_HPP
#define PUD_SEARCH_STEP_HPP

#include <cstdint>

// What one step of a PUD search did: unfolded a leaf (the atomic unit of work), reported a solution,
// or reported a refutation (a blocked leaf, or a conflict found when modules were loaded).
enum struct pud_search_step : uint8_t {
    unfolded,
    solved,
    refuted,
};

#endif
