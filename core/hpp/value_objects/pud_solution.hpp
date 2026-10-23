#ifndef PUD_SOLUTION_HPP
#define PUD_SOLUTION_HPP

#include <compare>
#include "value_objects/expr.hpp"
#include "value_objects/pud_node_id.hpp"

// A proven leaf of a conjecture (named by its root), and the conjecture's head read in that leaf's
// bindings.
struct pud_solution {
    pud_node_id conjecture;
    pud_node_id leaf;
    const expr* answer;
    auto operator<=>(const pud_solution&) const = default;
};

#endif
