#ifndef PUD_SPECIALIZATION_HPP
#define PUD_SPECIALIZATION_HPP

#include "expr.hpp"

struct pud_specialization {
    uint32_t var_idx;
    const expr* value;
};

#endif
