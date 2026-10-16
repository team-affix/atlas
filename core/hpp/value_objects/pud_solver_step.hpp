#ifndef PUD_SOLVER_STEP_HPP
#define PUD_SOLVER_STEP_HPP

#include <variant>
#include "value_objects/pud_solution.hpp"
#include "value_objects/pud_unfolded.hpp"

using pud_solver_step = std::variant<pud_unfolded, pud_solution>;

#endif
