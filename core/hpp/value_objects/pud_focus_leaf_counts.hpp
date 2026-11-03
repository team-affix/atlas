#ifndef PUD_FOCUS_LEAF_COUNTS_HPP
#define PUD_FOCUS_LEAF_COUNTS_HPP

#include <cstddef>

// How many live leaves sit under the focus, and how many of those no unfold is allowed on.
struct pud_focus_leaf_counts {
    size_t live;
    size_t blocked;
    auto operator<=>(const pud_focus_leaf_counts&) const = default;
};

#endif
