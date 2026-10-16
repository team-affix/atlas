#ifndef PUD_SOLUTION_HPP
#define PUD_SOLUTION_HPP

#include <compare>
#include "value_objects/pud_node_id.hpp"

struct pud_solution {
    pud_node_id leaf;
    auto operator<=>(const pud_solution&) const = default;
};

#endif
