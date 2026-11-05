#ifndef PUD_WITNESS_SEARCH_STACK_HPP
#define PUD_WITNESS_SEARCH_STACK_HPP

#include <cstddef>
#include <deque>
#include <optional>
#include "value_objects/pud_replay.hpp"

// One level of a depth-first witness search: the replay at the node the search stands on there (none while it
// still has to pick one), and the next child index of the level above to try.
struct pud_witness_search_frame {
    std::optional<pud_replay> current;
    size_t next_sibling;
    bool operator==(const pud_witness_search_frame&) const = default;
};

// The levels a witness search descended through below its root. An empty stack starts a new search.
struct pud_witness_search_stack {
    std::deque<pud_witness_search_frame> frames;
    bool operator==(const pud_witness_search_stack&) const = default;
};

#endif
