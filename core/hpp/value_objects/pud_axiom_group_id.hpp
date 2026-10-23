#ifndef PUD_AXIOM_GROUP_ID_HPP
#define PUD_AXIOM_GROUP_ID_HPP

#include <cstddef>

struct pud_axiom_group_id {
    size_t value;
    auto operator<=>(const pud_axiom_group_id&) const = default;
};

#endif
