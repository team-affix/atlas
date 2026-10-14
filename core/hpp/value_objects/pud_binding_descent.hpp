#ifndef PUD_BINDING_DESCENT_HPP
#define PUD_BINDING_DESCENT_HPP

#include "value_objects/pud_ubs_state.hpp"

struct pud_binding_descent {
    pud_ubs_state ubs_state;
    auto operator<=>(const pud_binding_descent&) const = default;
};

#endif
