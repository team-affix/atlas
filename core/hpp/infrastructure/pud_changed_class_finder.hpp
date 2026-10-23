#ifndef PUD_CHANGED_CLASS_FINDER_HPP
#define PUD_CHANGED_CLASS_FINDER_HPP

#include <vector>
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/ubs_class.hpp"

// Lists, by root, the classes of `before` that `after` changed, where `after` was edited from
// `before`. The two states share every untouched node, so the cost follows the number of edits.
struct pud_changed_class_finder {
    std::vector<ubs_class> find_changed_classes(const pud_ubs_state& before, const pud_ubs_state& after) const;
};

#endif
