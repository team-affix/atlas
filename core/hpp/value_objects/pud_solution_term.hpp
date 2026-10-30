#ifndef PUD_SOLUTION_TERM_HPP
#define PUD_SOLUTION_TERM_HPP

#include "value_objects/expr.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

struct pud_solution_term {
    const expr* head;
    pud_module_id module;
    pud_ubs_state bindings;
    bool operator==(const pud_solution_term&) const = default;
};

#endif
