#ifndef PUD_ADDED_UNIFICATION_HPP
#define PUD_ADDED_UNIFICATION_HPP

#include "value_objects/expr.hpp"

struct pud_added_unification {
    uint32_t var_idx;
    const expr* value;
};

#endif
