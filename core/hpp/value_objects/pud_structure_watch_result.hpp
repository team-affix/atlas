#ifndef PUD_STRUCTURE_WATCH_RESULT_HPP
#define PUD_STRUCTURE_WATCH_RESULT_HPP

#include <vector>
#include "value_objects/pud_watch_head_id.hpp"

// What descending a structure watch into a child found: the watched goals whose structure changed,
// the heads of the goals the child added, and the child's watch.
template<typename Descent>
struct pud_structure_watch_result {
    std::vector<pud_watch_head_id> structure_changes;
    std::vector<pud_watch_head_id> added_head_ids;
    Descent child_descent;
};

#endif
