#ifndef PUD_SUBSTITUTION_HPP
#define PUD_SUBSTITUTION_HPP

#include "expr.hpp"

struct pud_substitution {
    uint32_t var_idx;
    const expr* value;
};

#endif
